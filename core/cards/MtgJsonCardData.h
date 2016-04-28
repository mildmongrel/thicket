#ifndef MTGJSONCARDDATA_H
#define MTGJSONCARDDATA_H

#include "MtgJson.h"
#include "CardData.h"
#include "rapidjson/document.h"


class MtgJsonCardData : public CardData
{
public:

    MtgJsonCardData( const std::string& setCode, const rapidjson::Value& cardValue );

    virtual std::string getSetCode() const override { return mSetCode; }

    virtual std::string getName() const override { return mName; }

    virtual int getMultiverseId() const override { return mMultiverseId; }

    virtual int getCMC() const override { return mCMC; }

    virtual RarityType getRarity() const override { return mRarity; }

    virtual bool isSplit() const override { return mSplit; }

    virtual std::set<ColorType> getColors() const override { return mColorSet; }

    virtual std::set<std::string> getTypes() const override { return mTypesSet; }

private:

    static std::string parseName( const rapidjson::Value& cardValue );
    static int parseMultiverseId( const rapidjson::Value& cardValue );
    static int parseCMC( const rapidjson::Value& cardValue );
    static RarityType parseRarity( const rapidjson::Value& cardValue );
    static bool parseSplit( const rapidjson::Value& cardValue );
    static std::set<ColorType> parseColors( const rapidjson::Value& cardValue );
    static std::set<std::string> parseTypes( const rapidjson::Value& cardValue );

    const std::string           mSetCode;
    const std::string           mName;
    const int                   mMultiverseId;
    const int                   mCMC;
    const RarityType            mRarity;
    const bool                  mSplit;
    const std::set<ColorType>   mColorSet;
    const std::set<std::string> mTypesSet;
};

#endif // MTGJSONCARDDATA_H
