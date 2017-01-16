#include "CardViewerWidget.h"

#include <QVariant>
#include <QStyleOption>
#include <QPainter>

#include "qtutils_widget.h"
#include "StringUtil.h"

#include "CardData.h"
#include "ImageLoader.h"
#include "FlowLayout.h"
#include "CardWidget.h"

// The default background is white to hide the white corners on JPG cards
// returned by gatherer. 
const static QString gStyleSheet =
    "QWidget { background-color: white; }"
    "QWidget[alert=\"true\"] { background-color: #FF2828; }";

const static QString gLabelStyleSheet =
    "QLabel { background-color: white; }"
    "QLabel[alert=\"true\"] { color: white; background-color: #FF2828; }";


CardViewerWidget::CardViewerWidget( ImageLoaderFactory*    imageLoaderFactory,
                                    const Logging::Config& loggingConfig,
                                    QWidget*               parent )
  : QWidget( parent ),
    mImageLoaderFactory( imageLoaderFactory ),
    mFooterSpacing( 0 ),
    mZoomFactor( 1.0f ),
    mAlerted( false ),
    mCardsPreselectable( false ),
    mLoggingConfig( loggingConfig ),
    mLogger( loggingConfig.createLogger() )
{
    setStyleSheet( gStyleSheet );

    mLayout = new QVBoxLayout();

    // This will initialize the internal filtering.
    setFilters( {} );

    setLayout( mLayout );
}
 

CardViewerWidget::~CardViewerWidget()
{
}


void
CardViewerWidget::setCards( const QList<CardDataSharedPtr>& cards )
{
    mCardsList = cards;

    //
    // Performance gets bad if creating all new widgets from scratch,
    // especially when images are scaled.  Instead need to extract the
    // widgets from the layout, then only create new stuff as needed.
    //
    
    // Clear the filtered layouts of their widgets, but do not delete
    // the widgets.  (Still maintaining list of cardwidgets separately.)
    for( FlowLayout* layout : mFilteredCardsLayouts )
    {
        while( layout->count() > 0 ) 
        { 
            layout->takeAt( 0 ); 
        }
        layout->deleteLater();
    }
    mFilteredCardsLayouts.clear();

    // Clear our label-tracking list.
    mFilteredCardsLabels.clear();

    // Clear out the top-level layout - we're rebuilding it here.
    qtutils::clearLayout( mLayout );

    // This will contain all widgets once they're created/reused.
    QList<CardWidget*> newCardWidgetsList;

    //
    // Do the sorting and card list management on the big list up front.
    // As we filter, everything will stay sorted.
    //

    // Stable-sort our list here based on current sort functions.
    for( auto sortFunc : mSortFunctions )
    {
        std::stable_sort( mCardsList.begin(), mCardsList.end(), sortFunc );
    }

    // Add basic lands to the list here so they don't get sorted with the
    // other cards.
    QList<CardDataSharedPtr> displayedCardsList = mCardsList;
    for( auto basic : gBasicLandTypeArray )
    {
        for( int i = 0; i < mBasicLandQtys.getQuantity( basic ); ++i )
        {
            displayedCardsList.append( mBasicLandCardDataMap.getCardData( basic ) );
        }
    }

    for( auto filter : mFilters )
    {
        // Move matching cards into a filtered card list.
        QList<CardDataSharedPtr> filteredCardsList;
        QMutableListIterator<CardDataSharedPtr> i( displayedCardsList );
        while( i.hasNext() )
        {
            CardDataSharedPtr c = i.next();
            if( filter.filterFunc( c ) )
            {
                filteredCardsList.push_back( c );
                i.remove();
            }
        }

        // If nothing got filtered, move on to the next filter.
        if( filteredCardsList.empty() ) continue;

        // Unless we only have a single active filter (the "Other" filter),
        // show a label for the filtered cards. 
        if( mFilters.size() > 1 )
        {
            QLabel* label = new QLabel( QString::fromStdString( filter.name ) + " (" + QString::number( filteredCardsList.size() ) + ")" );
            label->setStyleSheet( gLabelStyleSheet );
            label->setProperty( "alert", mAlerted ? "true" : "false" );
            label->setAlignment( Qt::AlignHCenter | Qt::AlignTop );
            mFilteredCardsLabels.push_back( label );
            mLayout->addWidget( label );
        }

        FlowLayout* filteredCardsLayout = new FlowLayout();
        mFilteredCardsLayouts.push_back( filteredCardsLayout );
        mLayout->addLayout( filteredCardsLayout );
        for( auto cardDataSharedPtr : filteredCardsList )
        {
            // Look for an existing card widget that matches our card data,
            // and extract it from the list if found.
            CardWidget* cardWidget = nullptr;
            for( int i = 0; i < mCardWidgetsList.count(); ++i )
            {
                CardWidget* w = mCardWidgetsList[i];
                if( w->getCardData() == cardDataSharedPtr )
                {
                    cardWidget = mCardWidgetsList.takeAt( i );
                    mLogger->debug( "reusing CardWidget for name={} muid={}",
                            cardDataSharedPtr->getName(), cardDataSharedPtr->getMultiverseId() );
                    break;
                }
            }

            if( !cardWidget )
            {
                // The widget doesn't exist.  Create it.
                mLogger->debug( "creating CardWidget name={} muid={}", cardDataSharedPtr->getName(), cardDataSharedPtr->getMultiverseId() );
                QString card = QString::fromStdString( cardDataSharedPtr->getName() );

                cardWidget = new CardWidget( cardDataSharedPtr, mImageLoaderFactory, QSize( 223, 310 ),
                        mLoggingConfig.createChildConfig( "cardwidget" ) );
                cardWidget->setZoomFactor( mZoomFactor );
                cardWidget->setPreselectable( mCardsPreselectable );
                cardWidget->loadImage();
                connect(cardWidget, SIGNAL(preselectRequested()),
                        this, SLOT(handleCardPreselectRequested()));
                connect(cardWidget, SIGNAL(selectRequested()),
                        this, SLOT(handleCardSelectRequested()));
                connect(cardWidget, SIGNAL(moveRequested()),
                        this, SLOT(handleCardMoveRequested()));
                cardWidget->setContextMenuPolicy( Qt::CustomContextMenu );
                connect(cardWidget, SIGNAL(customContextMenuRequested(const QPoint&)),
                        this, SLOT(handleCardContextMenu(const QPoint&)));
            }
            filteredCardsLayout->addWidget( cardWidget );
            newCardWidgetsList.push_back( cardWidget );
        }

    }

    if( mFooterSpacing > 0 ) mLayout->addSpacing( mFooterSpacing );

    // This keeps everything pushed nicely to the top of the main area.
    mLayout->addStretch();

    // Clean up any cardwidgets remaining in the original list.
    for( auto w : mCardWidgetsList )
    {
        w->deleteLater();
    }
    mCardWidgetsList.clear();

    // Old is new.
    mCardWidgetsList = newCardWidgetsList;
}


void
CardViewerWidget::setBasicLandCardDataMap( const BasicLandCardDataMap& val )
{
    mBasicLandCardDataMap = val;

    // Reset the cards list which will reload card data (i.e. new images).
    setCards( mCardsList );
}


void
CardViewerWidget::setBasicLandQuantities( const BasicLandQuantities& basicLandQtys )
{
    mLogger->trace( "setting basic land qtys" );
    mBasicLandQtys = basicLandQtys;

    // Reset the cards list, sorting is done there.
    setCards( mCardsList );
}


void
CardViewerWidget::setZoomFactor( float zoomFactor )
{
    mLogger->trace( "setting zoom factor: {}", zoomFactor );
    mZoomFactor = zoomFactor;

    // Update the zoom for each CardWidget.
    for( auto cardWidget : mCardWidgetsList )
    {
        cardWidget->setZoomFactor( mZoomFactor );
    }
}


void
CardViewerWidget::setSortCriteria( const CardSortCriterionVector& sortCriteria )
{
    mLogger->trace( "setting sort criteria" );

    mSortFunctions.erase( mSortFunctions.begin(), mSortFunctions.end() );

    // The actual sorting is done in reverse order, so insert functions
    // in front.
    for( CardSortCriterionType criterion : sortCriteria )
    {
        switch( criterion )
        {
            case CARD_SORT_CRITERION_NAME:
                mSortFunctions.insert( mSortFunctions.begin(), compareCardDataName );
                break;
            case CARD_SORT_CRITERION_CMC:
                mSortFunctions.insert( mSortFunctions.begin(), compareCardDataCMCs );
                break;
            case CARD_SORT_CRITERION_COLOR:
                mSortFunctions.insert( mSortFunctions.begin(), compareCardDataColors );
                break;
            case CARD_SORT_CRITERION_RARITY:
                mSortFunctions.insert( mSortFunctions.begin(), compareCardDataRarity );
                break;
            case CARD_SORT_CRITERION_TYPE:
                mSortFunctions.insert( mSortFunctions.begin(), compareCardDataTypes );
                break;
            case CARD_SORT_CRITERION_NONE:
                break;
            default:
                mLogger->warn( "unknown sort criteria: {}", criterion );
                break;
        }
    }

    // Reset the cards list, sorting is done there.
    setCards( mCardsList );
}


void
CardViewerWidget::setAlert( bool alert )
{
    mAlerted = alert;

    // Change the property, which will allow a different look according
    // to the stylesheet setup.  Need to take some special actions for
    // this to work: https://wiki.qt.io/Dynamic_Properties_and_Stylesheets
    setProperty( "alert", alert ? "true" : "false" );
    style()->unpolish( this );
    style()->polish( this );
    update();

    for( auto label : mFilteredCardsLabels )
    {
        label->setProperty( "alert", alert ? "true" : "false" );
        label->style()->unpolish( label );
        label->style()->polish( label );
        label->update();
    }
}


void
CardViewerWidget::setCardsPreselectable( bool preselectable )
{
    mLogger->trace( "setting cards preselectable: {}", preselectable );

    for( auto w : mCardWidgetsList )
    {
        w->setPreselectable( preselectable );
    }

    mCardsPreselectable = preselectable;
}


void
CardViewerWidget::setCategorization( const CardCategorizationType& categorization )
{
    FilterVectorType filters;

    switch( categorization )
    {
        case CARD_CATEGORIZATION_NONE:

            mLogger->debug( "card categorization: none" );
            break;

        case CARD_CATEGORIZATION_CMC:

            mLogger->debug( "card categorization: cmc" );

            for( int cmc = 0; cmc < 20; ++cmc )
            {
                Filter f( "CMC " + std::to_string( cmc ),
                        [cmc]( const CardDataSharedPtr& cardDataSharedPtr )
                        {
                            return ( cmc == cardDataSharedPtr->getCMC() );
                        } );
                filters.push_back( f );
            }
            break;

        case CARD_CATEGORIZATION_COLOR:
        {
            mLogger->debug( "card categorization: color" );

            // Colorless filter.
            Filter colorlessFilter( "Colorless",
                    []( const CardDataSharedPtr& cardDataSharedPtr )
                    {
                        std::set<ColorType> colors = cardDataSharedPtr->getColors();
                        return colors.empty();
                    } );
            filters.push_back( colorlessFilter );

            // Single-color filters.
            for( ColorType color : gColorTypeArray )
            {
                Filter f( stringify( color ),
                        [color]( const CardDataSharedPtr& cardDataSharedPtr )
                        {
                            std::set<ColorType> colors = cardDataSharedPtr->getColors();
                            return (colors.size() == 1) && (*colors.begin() == color);
                        } );
                filters.push_back( f );
            }

            // Multicolor filter.
            Filter multicolorFilter( "Multicolor",
                    []( const CardDataSharedPtr& cardDataSharedPtr )
                    {
                        std::set<ColorType> colors = cardDataSharedPtr->getColors();
                        return (colors.size() > 1);
                    } );
            filters.push_back( multicolorFilter );
            break;
        }

        case CARD_CATEGORIZATION_RARITY:

            mLogger->debug( "card categorization: rarity" );

            for( RarityType rarity : gRarityTypeArray )
            {
                Filter f( stringify( rarity ),
                        [rarity]( const CardDataSharedPtr& cardDataSharedPtr )
                        {
                            return ( rarity == cardDataSharedPtr->getRarity() );
                        } );
                filters.push_back( f );
            }
            break;

        case CARD_CATEGORIZATION_TYPE:
        {
            mLogger->debug( "card categorization: type" );

            // Filters for items with a single type.
            std::vector<std::string> allTypes = { "Land", "Artifact", "Creature", "Enchantment", "Instant", "Sorcery", "Planeswalker" };
            for( auto type : allTypes )
            {
                Filter f( type,
                        [type]( const CardDataSharedPtr& cardDataSharedPtr )
                        {
                            std::set<std::string> cardTypes = cardDataSharedPtr->getTypes();
                            return (cardTypes.size() == 1) && StringUtil::icompare( type, *cardTypes.begin() );
                        } );
                filters.push_back( f );
            }

            // Filter for multi-type cards.
            Filter f( "Multi-type",
                    []( const CardDataSharedPtr& cardDataSharedPtr )
                    {
                        std::set<std::string> cardTypes = cardDataSharedPtr->getTypes();
                        return (cardTypes.size() > 1);
                    } );
            filters.push_back( f );

            break;
        }

        default:

            mLogger->warn( "card categorization: {} (unimplemented)", categorization );
            break;

    }

    setFilters( filters );
}


// Set the filters for displaying cards by category.  Each filter catches
// cards, with leftovers being potentially captured by the remaining filters
// in order.  Anything not caught by a submitted filter gets placed into
// an "other" category,
void
CardViewerWidget::setFilters( const FilterVectorType& filters )
{
    mFilters = filters;

    // The internal list must always have a filter at the end that
    // catches everything.
    Filter f( "Other", [](const CardDataSharedPtr&) { return true; } );
    mFilters.push_back( f );

    setCards( mCardsList );
}


void
CardViewerWidget::setPreselected( CardWidget* cardWidget )
{
    // Disable preselection for all cards but the one requested.
    for( auto w : mCardWidgetsList )
    {
        w->setPreselected( w == cardWidget );
    }
}


// Overridden to ensure proper stylesheet handling.  Without this the
// dynamic background changes aren't performed properly.
// See http://www.qtcentre.org/threads/37976-Q_OBJECT-and-CSS-background-image
void
CardViewerWidget::paintEvent( QPaintEvent *pe )
{                                                                                                                                        
    QStyleOption o;
    o.initFrom( this );
    QPainter p( this );
    style()->drawPrimitive( QStyle::PE_Widget, &o, &p, this );
};


void
CardViewerWidget::handleCardPreselectRequested()
{
    CardWidget *cardWidget = qobject_cast<CardWidget*>(QObject::sender());
    CardDataSharedPtr cardData = cardWidget->getCardData();
    mLogger->debug( "card preselect requested: {} {}", (std::size_t) cardWidget, cardData->getName() );
    emit cardPreselectRequested( cardWidget, cardData );
}


void
CardViewerWidget::handleCardSelectRequested()
{
    CardWidget *cardWidget = qobject_cast<CardWidget*>(QObject::sender());
    CardDataSharedPtr cardData = cardWidget->getCardData();
    mLogger->debug( "card select requested: {} {}", (std::size_t) cardWidget, cardData->getName() );
    emit cardSelectRequested( cardData );
}


void
CardViewerWidget::handleCardMoveRequested()
{
    CardWidget *cardWidget = qobject_cast<CardWidget*>(QObject::sender());
    CardDataSharedPtr cardData = cardWidget->getCardData();
    mLogger->debug( "card move requested: {} {}", (std::size_t) cardWidget, cardData->getName() );
    emit cardMoveRequested( cardData );
}


void
CardViewerWidget::handleCardContextMenu( const QPoint& pos )
{
    CardWidget *cardWidget = qobject_cast<CardWidget*>(QObject::sender());
    CardDataSharedPtr cardData = cardWidget->getCardData();
    mLogger->debug( "card context menu: {} {}", (std::size_t) cardWidget, cardData->getName() );

    // map position from cardwidget to this widget
    const QPoint thisPos = cardWidget->mapTo( this, pos );

    emit cardContextMenuRequested( cardWidget, cardData, thisPos );
}


bool
CardViewerWidget::compareCardDataColors( CardDataSharedPtr a, CardDataSharedPtr b )
{   
    std::set<ColorType> colorsA = a->getColors();
    std::set<ColorType> colorsB = b->getColors();

    if( (colorsA.size() > 1) && (colorsB.size() > 1) )
    {
        // Both cards are multicolor, order by first color.
        return *(colorsA.begin()) < *(colorsB.begin());
    }
    else if( colorsA.size() == colorsB.size() )
    {
        if( !colorsA.empty() )
        {
            // Both cards are single-color, order by first color.
            return *(colorsA.begin()) < *(colorsB.begin());
        }
        else
        {
            // Both cards are colorless, no ordering here.
            return false;
        }
    }
    else
    {
        // Card are different color quantities and not both multicolor, so
        // order by quantity, i.e. colorless < single-color < multicolor.
        return (colorsA.size() < colorsB.size());
    }
}   


bool
CardViewerWidget::compareCardDataTypes( CardDataSharedPtr a, CardDataSharedPtr b )
{   
    std::set<std::string> typesA = a->getTypes();
    std::set<std::string> typesB = b->getTypes();

    if( !typesA.empty() && !typesB.empty() )
    {
        // Both cards have at least one type, order by first type.
        return *(typesA.begin()) < *(typesB.begin());
    }
    else
    {
        // If either type-set is empty then just compare sizes.
        return typesA.size() < typesB.size();
    }
}   
