#ifndef LIBGEN_CONFIG_H
#define LIBGEN_CONFIG_H

#include <string>
#include <vector>
#include <set>

class Config
{
public:
    class Values
    {
    public:
        typedef std::vector<std::string> StringList;
        typedef std::set<std::string> StringSet;

        typedef enum { CUST_NONE=0, CUST_STA, CUST_CANDW, CUST_MAX } CustCodes;

    public:
        static const int MAX_MISC_VALUES = 10;

    public:
        //int instanceId;             ///< Instance id of stack user
        //std::string activeEmIp;     ///< IP Address of Active Element Manager
        //int activeEmPort;           ///< Port Number of Active Element Manager
        //std::string standbyEmIp;    ///< IP Address of Standby Element Manager
        //int standbyEmPort;          ///< Port Number of Standby Element Manager
        //std::string localIp;        ///< IP Address of this machine
        //std::string stackLogDir;    ///< Directory for stack logging, if enabled

        bool active;                    ///< Whether this is active SVR initially

        bool mapRegisterAssoc;          ///< Whether to register MAP users only when both associated SAP/SSNs are available
        StringList mapUsers;            ///< Definition of the MAP users
        bool mapAutoAssoc;              ///< Whether to associate all MAP users on other SAPs or define manually below
        StringList mapUserAssocs;       ///< Definition of the MAP user associations

        std::string realHlrAddr;            ///< Address we send as forwarded address in MapOpenResponse
        unsigned char realHlrAddrBcd[32];   ///< BCD version of forwarded addresses
        ssize_t realHlrAddrBcdLen;          ///< Length of BCD version of forwarded addresses
        std::string realHlrAddrIsdn;        ///< ISDN format of the above
        unsigned char realHlrAddrIsdnBcd[32];///< BCD version of forwarded addresses
        ssize_t realHlrAddrIsdnBcdLen;      ///< Length of BCD version of forwarded addresses

        std::string fakeHlrAddr;            ///< Address we send as responding address in MapOpenResponse
        unsigned char fakeHlrAddrBcd[32];   ///< BCD version of responding addresses
        ssize_t fakeHlrAddrBcdLen;          ///< Length of BCD version of responding addresses
        std::string fakeHlrAddrIsdn;        ///< ISDN format of the above
        unsigned char fakeHlrAddrIsdnBcd[32];///< BCD version of responding addresses
        ssize_t fakeHlrAddrIsdnBcdLen;      ///< Length of BCD version of responding addresses

        std::string engineAddr;         ///< Address of ENGINE
        unsigned char engineAddrBcd[32];///< BCD version of SCF addresses
        ssize_t engineAddrBcdLen;       ///< Length of BCD version of SCF addresses
        std::string engineAddrIsdn;     ///< ISDN format of the above
        unsigned char engineAddrIsdnBcd[32];///< BCD version of SCF addresses
        ssize_t engineAddrIsdnBcdLen;   ///< Length of BCD version of SCF addresses

        StringList translationTypes;    ///< List of translation type changes to apply

        StringSet imsiWhitelist;        ///< Set allowed IMSIs, if empty all are allowed

        int peerHeartbeatPort;          ///< Port for heartbeat communications
        int peerHeartbeatInterval;      ///< How often, in milliseconds, to send heartbeats to peer
        int peerHeartbeatTimeout;       ///< How long to wait, in milliseconds, before considering peer dead

        int ldNodeId;                   ///< The LD node id
        std::string ldNodeName;         ///< The LD node name
        int ldMessageRespTimeout;       ///< How long (in seconds) to wait for responses from the LD/SLEE, 0 disables
        StringSet ldConnections;        ///< Set of host:port LD connections to use

        int maxDeregisterWait;          ///< Maximum time to wait for MAP user deregistration confirm messages
        int maxDialogAge;               ///< How old a dialog has to be, in seconds, before it is considered abandoned
        int dialogPurgeProb;            ///< What percentage chance of purging old dialogs for each call
        bool dumpMessageFiles;          ///< Whether to write out message files
        std::string msgsDirRoot;        ///< Where to store message files if enabled by previous option
        int messageCountInterval;       ///< How often to output the current message total to debug

        bool dumpCoreFiles;             ///< Whether to dump core files on errors instead of backtrace in logs

        CustCodes custCode;             ///< Who the current customer is

        struct {
            bool    preferredNetwork;
            bool    borderRoaming;
            bool    welcomeSMS;
            bool    abroadSMS;
            bool    voicemailTromboning;
            bool    smsFraud;
        } activeServices;           ///< Which services are currently available on the platform

        int miscIntValues[MAX_MISC_VALUES]; ///< Temporary values for hackery pokery
        std::string miscStrValues[MAX_MISC_VALUES]; ///< Temporary values for hackery pokery

    public:
        Values();

        void assignDefaults();
        bool setValue( const std::string name, const std::string value );
    };

private:
    std::string mFile;

public:
    mutable Values  values;

public:
    Config( const std::string &file );

    bool readFile() const;
};

extern std::string gConfigFile;
extern const Config *gConfig;

class ConfigRegistration
{
public:
    ConfigRegistration();
};

#endif // LIBGEN_CONFIG_H
