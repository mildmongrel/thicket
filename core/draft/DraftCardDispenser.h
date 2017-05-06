#ifndef DRAFTCARDDISPENSER_H
#define DRAFTCARDDISPENSER_H

#include <vector>
#include <string>
#include <memory>

template< typename TCardDescriptor = std::string >
class DraftCardDispenser
{
public:
    virtual std::vector<TCardDescriptor> dispense( unsigned int qty ) = 0;
    virtual std::vector<TCardDescriptor> dispenseAll() = 0;
};

//
// These act like typedefs but allows passing through template types.
//

template< typename TCardDescriptor = std::string > using
DraftCardDispenserSharedPtr = std::shared_ptr<DraftCardDispenser<TCardDescriptor>>;

template< typename TCardDescriptor = std::string > using
DraftCardDispenserSharedPtrVector = std::vector<DraftCardDispenserSharedPtr<TCardDescriptor>>;

#endif
