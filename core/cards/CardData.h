#ifndef CARDDATA_H
#define CARDDATA_H

#include "CardDataTypes.h"
#include <set>

class CardData
{
public:
    virtual ~CardData() {}
    virtual std::string getSetCode() const = 0;
    virtual std::string getName() const = 0;
    virtual int getMultiverseId() const = 0;
    virtual int getCMC() const = 0;
    virtual RarityType getRarity() const = 0;
    virtual bool isSplit() const = 0;
    virtual std::set<ColorType> getColors() const = 0;
    virtual bool isMulticolor() const { return getColors().size() > 1; }
    virtual std::set<std::string> getTypes() const = 0;
    virtual size_t getHashValue() const;
};

inline size_t CardData::getHashValue() const
{
    // Not perfect, but good enough.
    size_t h1 = std::hash<std::string>{}( getSetCode() );
    size_t h2 = std::hash<std::string>{}( getName() );
    return h1 ^ h2;  
}

inline bool operator==( const CardData& a, const CardData& b )
{
    return (a.getName() == b.getName()) && (a.getSetCode() == b.getSetCode());
}

inline bool operator<( const CardData& a, const CardData& b )
{
    return (a.getName() == b.getName()) ? (a.getSetCode() < b.getSetCode())
                                        : (a.getName() < b.getName());
}

namespace std {
    template <> struct hash<CardData>
    {
        size_t operator()( const CardData& c ) const { return c.getHashValue(); }
    };
}

#endif // CARDDATA_H
