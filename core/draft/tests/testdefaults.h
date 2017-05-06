#ifndef TESTDEFAULTS_H
#define TESTDEFAULTS_H

#include "Draft.h"
#include "DraftConfig.pb.h"

class TestDefaults
{
public:

    class TestCardDispenser : public DraftCardDispenser<std::string>
    {
    public:
        TestCardDispenser( const std::string& setCode = std::string() )
          : mSetCode( setCode )
        {
            reset();
        }

        std::vector<std::string> dispense( unsigned int qty )
        {
            std::vector<std::string> vec;
            for( unsigned int i = 0; i < qty; ++i )
            {
                vec.push_back( mCards.front() );
                mCards.pop_front();
                if( mCards.empty() ) reset();
            }
            return vec;
        }

        std::vector<std::string> dispenseAll()
        {
            std::vector<std::string> vec;
            std::copy( mCards.begin(), mCards.end(), std::back_inserter( vec ) );
            reset();
            return vec;
        }

    private:

        void reset()
        {
            mCards.clear();
            for( int i = 0; i < 15; ++i )
            {
                mCards.push_back( mSetCode + std::string(":card") + std::to_string(i) );
            }
        }

        const std::string mSetCode;
        std::deque<std::string> mCards;
    };

    static inline DraftCardDispenserSharedPtrVector<> getDispensers( int qty = 1 )
    {
        DraftCardDispenserSharedPtrVector<> dispensers;
        for( int i = 0; i < qty; ++i )
        {
            auto dispSptr = std::make_shared<TestCardDispenser>( std::to_string(i) );
            dispensers.push_back( dispSptr );
        }
        return dispensers;
    }

    static inline proto::DraftConfig getSimpleBoosterDraftConfig( int rounds,
                                                                  int chairs,
                                                                  int roundSelectionTime = 30 )
    {
        proto::DraftConfig dc;
        dc.set_chair_count( chairs );
        proto::DraftConfig::CardDispenser* dispenser = dc.add_dispensers();
        dispenser->add_source_booster_set_codes( "" );
        for( int r = 0; r < rounds; ++r )
        {
            proto::DraftConfig::Round* round = dc.add_rounds();
            proto::DraftConfig::BoosterRound* boosterRound = round->mutable_booster_round();
            boosterRound->set_selection_time( roundSelectionTime );
            boosterRound->set_pass_direction( (r%2) == 0 ?
                    proto::DraftConfig::DIRECTION_CLOCKWISE :
                    proto::DraftConfig::DIRECTION_COUNTER_CLOCKWISE );
            proto::DraftConfig::CardDispensation* dispensation = boosterRound->add_dispensations();
            dispensation->set_dispenser_index( 0 );
            dispensation->set_dispense_all( true );
            for( int i = 0; i < chairs; ++i )
            {
                dispensation->add_chair_indices( i );
            }
        }
        return dc;
    }

    static inline proto::DraftConfig getSimpleSealedDraftConfig( int chairs,
                                                                 int postRoundTime = 0 )
    {
        const int DISPENSERS = 6;

        proto::DraftConfig dc;
        dc.set_chair_count( chairs );
        for( int d = 0; d < DISPENSERS; ++d )
        {
            proto::DraftConfig::CardDispenser* dispenser = dc.add_dispensers();
            dispenser->add_source_booster_set_codes( "" );
        }

        proto::DraftConfig::Round* round = dc.add_rounds();
        round->set_post_round_timer( postRoundTime );
        proto::DraftConfig::SealedRound* sealedRound = round->mutable_sealed_round();
        for( int d = 0; d < DISPENSERS; ++d )
        {
            proto::DraftConfig::CardDispensation* dispensation = sealedRound->add_dispensations();
            dispensation->set_dispenser_index( d );
            dispensation->set_dispense_all( true );
            for( int i = 0; i < chairs; ++i )
            {
                dispensation->add_chair_indices( i );
            }
        }

        return dc;
    }


    static inline proto::DraftConfig getSimpleGridDraftConfig( int rounds        = 18,
                                                               int chairs        = 2,
                                                               int selectionTime = 30,
                                                               int postRoundTime = 0 )
    {
        proto::DraftConfig dc;
        dc.set_chair_count( chairs );

        proto::DraftConfig::CardDispenser* dispenser = dc.add_dispensers();
        dispenser->add_source_booster_set_codes( "" );

        for( int r = 0; r < rounds; ++r )
        {
            proto::DraftConfig::Round* round = dc.add_rounds();
            round->set_post_round_timer( postRoundTime );
            proto::DraftConfig::GridRound* gridRound = round->mutable_grid_round();

            gridRound->set_dispenser_index( 0 );
            gridRound->set_initial_chair( r % chairs );
            gridRound->set_selection_time( selectionTime );
        }

        return dc;
    }
};

#endif
