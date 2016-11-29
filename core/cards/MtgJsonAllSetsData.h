#ifndef MTGJSONALLSETSDATA_H
#define MTGJSONALLSETSDATA_H

#include "AllSetsData.h"
#include "rapidjson/document.h"
#include "lrucache.hpp"
#include <string>
#include <set>
#include <vector>
#include "Logging.h"

class MtgJsonAllSetsData : public AllSetsData
{
public:

    static const unsigned int DEFAULT_CACHE_SIZE = 100;

    MtgJsonAllSetsData( unsigned int    cacheSize,
                        Logging::Config loggingConfig = Logging::Config() );

    MtgJsonAllSetsData( Logging::Config loggingConfig = Logging::Config() )
      : MtgJsonAllSetsData( DEFAULT_CACHE_SIZE, loggingConfig ) {}

    virtual ~MtgJsonAllSetsData() {}

    bool parse( FILE* fp );

    virtual std::vector<std::string> getSetCodes() const override;
    virtual std::string getSetName( const std::string& code, const std::string& defaultName = "") const override;
    virtual bool hasBoosterSlots( const std::string& code ) const override;
    virtual std::vector<SlotType> getBoosterSlots( const std::string& code ) const override;
    virtual std::multimap<RarityType,std::string> getCardPool( const std::string& code ) const override;

    // Create a CardData object given set code and name.  If the set code is empty, search
    // through known sets in reverse chronological order for a name match.  Returns nullptr
    // if no match.
    virtual CardData* createCardData( const std::string& code, const std::string& name ) const override;

    virtual CardData* createCardData( int multiverseId ) const override;

    unsigned int getSetNameLookupCacheHits() const { return mCardLRUCacheHits; }
    unsigned int getSetNameLookupCacheMisses() const { return mCardLRUCacheMisses; }

private:

    using CardCacheKey = std::string;
    static CardCacheKey createCardCacheKey( const std::string& code, const std::string& name ) {
        return code + "_" + name;
    }

    struct CardCacheValue
    {
        CardCacheValue( const std::string&                          zSetCode,
                        const rapidjson::Value::ConstValueIterator& zCardIter )
          : setCode( zSetCode ), cardIter( zCardIter ) {}

        std::string                          setCode;
        rapidjson::Value::ConstValueIterator cardIter;
    private:
        CardCacheValue() {}
    };

    using CardLRUCache = cache::lru_cache<CardCacheKey,CardCacheValue>;

    // Find a card name given a range of rapidjson value iterators.  Returns
    // iterator to found element or last if not found.
    rapidjson::Value::ConstValueIterator findCardValueByName(
                           rapidjson::Value::ConstValueIterator first,
                           rapidjson::Value::ConstValueIterator last,
                           const std::string&                   name ) const;

    rapidjson::Document mDoc;
    std::set<std::string> mAllSetCodes;
    std::vector<std::string> mSearchPrioritizedAllSetCodes;
    std::set<std::string> mBoosterSetCodes;

    mutable CardLRUCache mCardLRUCache;
    mutable unsigned int mCardLRUCacheHits;
    mutable unsigned int mCardLRUCacheMisses;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // MTGJSONALLSETSDATA_H
