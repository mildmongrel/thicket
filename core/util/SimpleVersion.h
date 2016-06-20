#ifndef SIMPLEVERSION_H
#define SIMPLEVERSION_H

class SimpleVersion
{
public:
    SimpleVersion( unsigned int maj = 0, unsigned int min = 0 )
      : mVer( maj, min ) {}

    unsigned int getMajor() const { return mVer.first; }
    unsigned int getMinor() const { return mVer.second; }

    bool olderThan( unsigned int maj, unsigned int min ) const
    {
        return mVer < std::make_pair(maj,min);
    }

    bool olderThan( const SimpleVersion& rhs ) const
    {
        return mVer < rhs.mVer;
    }

private:
    std::pair<unsigned int, unsigned int> mVer;
};


inline std::string
stringify( const SimpleVersion& ver )
{
    return std::to_string( ver.getMajor() ) + "." + std::to_string( ver.getMinor() );
}

#endif
