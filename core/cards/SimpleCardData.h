#ifndef SIMPLECARDDATA_H
#define SIMPLECARDDATA_H

#include "CardData.h"

// A class to substitute for CardData if it can't be derived elsewhere.
class SimpleCardData : public CardData
{
public:

    SimpleCardData( const std::string& name, const std::string& setCode = std::string() )
      : mName( name ),
        mSetCode( setCode ),
        mMultiverseId( -1 ),
        mCMC( -1 ),
        mRarity( RARITY_UNKNOWN )
    {}

    virtual std::string getName() const override { return mName; }
    virtual std::string getSetCode() const override { return mSetCode; }
    virtual int getMultiverseId() const override { return mMultiverseId; }
    virtual int getCMC() const override { return mCMC; }
    virtual RarityType getRarity() const override { return mRarity; }
    virtual bool isSplit() const override { return false; };
    virtual std::set<ColorType> getColors() const override { return std::set<ColorType>(); } // empty set
    virtual std::set<std::string> getTypes() const override { return std::set<std::string>(); } // empty set

private:
    std::string mName;
    std::string mSetCode;
    int mMultiverseId;
    int mCMC;
    RarityType mRarity;
};

namespace std {
    template <> struct hash<SimpleCardData>
    {
        size_t operator()( const SimpleCardData& c ) const { return c.getHashValue(); }
    };
}

#endif
