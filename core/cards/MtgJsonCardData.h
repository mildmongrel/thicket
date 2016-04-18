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

    virtual std::string getName() const override;

    virtual int getMultiverseId() const override;

    virtual int getCMC() const override;

    virtual RarityType getRarity() const override;

    virtual bool isSplit() const override;

    virtual std::set<ColorType> getColors() const override { return mColorSet; }

    virtual std::set<std::string> getTypes() const override { return mTypesSet; }

private:

    std::set<ColorType> parseColors() const;
    std::set<std::string> parseTypes() const;

    const std::string           mSetCode;
    const rapidjson::Value&     mCardValue;
    const std::set<ColorType>   mColorSet;
    const std::set<std::string> mTypesSet;
};

#endif // MTGJSONCARDDATA_H
