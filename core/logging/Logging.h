#ifndef LOGGING_H
#define LOGGING_H

#include "spdlog/spdlog.h"
#include "spdlog/sinks/null_sink.h"

namespace Logging
{
    inline std::shared_ptr<spdlog::logger> createNullLogger()
    {
        auto sink = std::make_shared<spdlog::sinks::null_sink_st>();
        auto logger = std::make_shared<spdlog::logger>( "", sink );
        return logger;
    }

    class Config
    {
    public:

        // Default constructor creates a do-nothing configuration.
        Config()
          : mStdoutLogging( false ),
            mSimpleFileName(),
            mRotatingFileBaseName(),
            mRotatingFileExtension(),
            mRotatingFileSizeLimit( 0 ),
            mRotatingFileCount( 0 ),
            mAppendThisAddr( false ),
            mLevel( spdlog::level::off )
        {}

        void setName( const std::string& name ) { mName = name; }
        void setLevel( const spdlog::level::level_enum& level ) { mLevel = level; }
        void setStdoutLogging( bool enabled ) { mStdoutLogging = enabled; }
        void setSimpleFileLogging( const std::string& fileName )
        {
            mSimpleFileName = fileName;
        };
        void setRotatingFileLogging( const std::string& rotatingFileBaseName,
                                     const std::string& rotatingFileExtension,
                                     int                rotatingFileSizeLimit,
                                     int                rotatingFileCount)
        {
            mRotatingFileBaseName = rotatingFileBaseName;
            mRotatingFileExtension = rotatingFileExtension;
            mRotatingFileSizeLimit = rotatingFileSizeLimit;
            mRotatingFileCount = rotatingFileCount;
        }

        void setAppendThisAddr( bool enabled ) { mAppendThisAddr = enabled; }

        // create a logger that complies with this policy
        // return a null logger if no logger needed
        std::shared_ptr<spdlog::logger> createLogger() const
        {
            if( !mStdoutLogging && mSimpleFileName.empty() && mRotatingFileBaseName.empty() )
            {
                return createNullLogger();
            }
            else
            {
                std::vector<spdlog::sink_ptr> sinks;
                if( mStdoutLogging )
                {
                    sinks.push_back( std::make_shared<spdlog::sinks::stdout_sink_st>() );
                }
                if( !mSimpleFileName.empty() )
                {
                    sinks.push_back( getSimpleFileSink( mSimpleFileName ) );
                }
                if( !mRotatingFileBaseName.empty() )
                {
                    sinks.push_back( getRotatingFileSink(
                            mRotatingFileBaseName, mRotatingFileExtension,
                            mRotatingFileSizeLimit, mRotatingFileCount ) );
                }
                auto logger = std::make_shared<spdlog::logger>( mName, begin(sinks), end(sinks) );
                logger->set_level( mLevel );
                logger->set_pattern( "[%L %n] %v" );
                return logger;
            }

        }

        // copies config but adds child name to this name
        Config createChildConfig( const std::string& childName ) const
        {
            Config child( *this );
            child.setName( mName + ":" + childName );
            return child;
        }

    private:

        static spdlog::sink_ptr getSimpleFileSink( const std::string& fileName )
        {
            static std::map<std::string,spdlog::sink_ptr> sinkMap;
            if( sinkMap.count( fileName ) == 0 )
            {
                sinkMap[fileName] = std::make_shared<spdlog::sinks::simple_file_sink_st>(
                        fileName, true );
            }
            return sinkMap[fileName];
        }

        static spdlog::sink_ptr getRotatingFileSink( const std::string& fileBaseName,
                                                     const std::string& fileExtension,
                                                     unsigned int       fileSizeLimit,
                                                     unsigned int       fileCount )
        {
            static std::map<std::string,spdlog::sink_ptr> sinkMap;
            if( sinkMap.count( fileBaseName ) == 0 )
            {
                sinkMap[fileBaseName] = std::make_shared<spdlog::sinks::rotating_file_sink_st>(
                        fileBaseName, fileExtension, fileSizeLimit, fileCount, true );
            }
            return sinkMap[fileBaseName];
        }

        std::string mName;
        bool mStdoutLogging;
        std::string mSimpleFileName;
        std::string mRotatingFileBaseName;
        std::string mRotatingFileExtension;
        unsigned int mRotatingFileSizeLimit;
        unsigned int mRotatingFileCount;
        bool mAppendThisAddr;
        spdlog::level::level_enum mLevel;
    };

};

#endif  // LOGGING_H
