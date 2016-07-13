#ifndef DRAFTTYPES_H
#define DRAFTTYPES_H

#include "Draft.h"
#include "DraftChairObserver.h"
#include <iostream>

struct DraftCard
{
    DraftCard( const std::string& n, const std::string& sc ) : name( n ), setCode( sc ) {}
    std::string name;
    std::string setCode;
};

inline bool operator==( const DraftCard& a, const DraftCard& b )
{
    return (a.name == b.name) && (a.setCode == b.setCode);
}

inline std::ostream& operator<<( std::ostream& os, const DraftCard& d )
{
    os << '[' << d.setCode << ',' << d.name << ']';
    return os;
}

typedef Draft<DraftCard> DraftType;
typedef DraftType::Observer DraftObserverType;
typedef DraftChairObserver<DraftCard> DraftChairObserverType;

#endif
