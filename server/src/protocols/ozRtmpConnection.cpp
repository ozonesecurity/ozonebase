#include "../base/oz.h"
#include "ozRtmpConnection.h"

#include "ozRtmp.h"
#include "ozRtmpSession.h"
#include "ozRtmpAmf.h"
#include "../libgen/libgenUtils.h"

#include <sys/time.h>
#include <sys/times.h>

#if 0
#include <gcrypt.h>
#ifndef SHA256_DIGEST_LENGTH
#define SHA256_DIGEST_LENGTH    32
#endif
#define HMAC_CTX    gcry_md_hd_t
#define HMAC_setup(ctx, key, len)   gcry_md_open(&ctx, GCRY_MD_SHA256, GCRY_MD_FLAG_HMAC); gcry_md_setkey(ctx, key, len)
#define HMAC_crunch(ctx, buf, len)  gcry_md_write(ctx, buf, len)
#define HMAC_finish(ctx, dig, dlen) dlen = SHA256_DIGEST_LENGTH; memcpy(dig, gcry_md_read(ctx, 0), dlen); gcry_md_close(ctx)
#else
#include <openssl/sha.h>
#include <openssl/hmac.h>

#define HMAC_setup(ctx, key, len)   HMAC_CTX_init(&ctx); HMAC_Init_ex(&ctx, key, len, EVP_sha256(), 0)
#define HMAC_crunch(ctx, buf, len)  HMAC_Update(&ctx, buf, len)
#define HMAC_finish(ctx, dig, dlen) HMAC_Final(&ctx, dig, &dlen); HMAC_CTX_cleanup(&ctx)
#endif

#include <assert.h>

#define ZM_RTMP_SIG_SIZE 1536

static const uint8_t GenuineFMSKey[] = {
  0x47, 0x65, 0x6e, 0x75, 0x69, 0x6e, 0x65, 0x20, 0x41, 0x64, 0x6f, 0x62,
    0x65, 0x20, 0x46, 0x6c,
  0x61, 0x73, 0x68, 0x20, 0x4d, 0x65, 0x64, 0x69, 0x61, 0x20, 0x53, 0x65,
    0x72, 0x76, 0x65, 0x72,
  0x20, 0x30, 0x30, 0x31,   /* Genuine Adobe Flash Media Server 001 */

  0xf0, 0xee, 0xc2, 0x4a, 0x80, 0x68, 0xbe, 0xe8, 0x2e, 0x00, 0xd0, 0xd1,
  0x02, 0x9e, 0x7e, 0x57, 0x6e, 0xec, 0x5d, 0x2d, 0x29, 0x80, 0x6f, 0xab,
    0x93, 0xb8, 0xe6, 0x36,
  0xcf, 0xeb, 0x31, 0xae
};              /* 68 */

static const uint8_t GenuineFPKey[] = {
  0x47, 0x65, 0x6E, 0x75, 0x69, 0x6E, 0x65, 0x20, 0x41, 0x64, 0x6F, 0x62,
    0x65, 0x20, 0x46, 0x6C,
  0x61, 0x73, 0x68, 0x20, 0x50, 0x6C, 0x61, 0x79, 0x65, 0x72, 0x20, 0x30,
    0x30, 0x31,         /* Genuine Adobe Flash Player 001 */
  0xF0, 0xEE,
  0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1, 0x02, 0x9E,
    0x7E, 0x57, 0x6E, 0xEC,
  0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB, 0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB,
    0x31, 0xAE
};              /* 62 */

static unsigned int calculateDigestOffset( uint8_t *handshake, unsigned int len, int method )
{
    unsigned int offset = 0;
    unsigned int result = 0;

    switch( method )
    {
        case 1 :
        {
            uint8_t *ptr = handshake + 8;

            offset += (*ptr);
            ptr++;
            offset += (*ptr);
            ptr++;
            offset += (*ptr);
            ptr++;
            offset += (*ptr);

            result = (offset % 728) + 12;

            if ( result + 32 > 771 )
                Fatal( "Unable to calculate valid digest offset for method %d, result is %d", method, result );
            break;
        }
        case 2 :
        {
            uint8_t *ptr = handshake + 772;

            offset += (*ptr);
            ptr++;
            offset += (*ptr);
            ptr++;
            offset += (*ptr);
            ptr++;
            offset += (*ptr);

            result = (offset % 728) + 776;

            if ( result + 32 > 1535 )
                Fatal( "Unable to calculate valid digest offset for method %d, result is %d", method, result );
            break;
        }
        default :
        {
            Panic( "Unexpected digest offset method %d", method );
        }
    }
    return( result );
}

static void HMACsha256( const uint8_t *message, size_t messageLen, const uint8_t *key, size_t keylen, uint8_t *digest )
{
    unsigned int digestLen;
    HMAC_CTX ctx;

    HMAC_setup(ctx, key, keylen);
    HMAC_crunch(ctx, message, messageLen);
    HMAC_finish(ctx, digest, digestLen);

    if ( digestLen != 32 )
      Panic( "Invalid digest length %d", digestLen );
}

static void calculateDigest( unsigned int digestPos, uint8_t *handshakeMessage, const uint8_t *key, size_t keyLen, uint8_t *digest )
{
    const int messageLen = ZM_RTMP_SIG_SIZE - SHA256_DIGEST_LENGTH;
    uint8_t message[ZM_RTMP_SIG_SIZE - SHA256_DIGEST_LENGTH];

    memcpy( message, handshakeMessage, digestPos );
    memcpy( message + digestPos, &handshakeMessage[digestPos + SHA256_DIGEST_LENGTH], messageLen - digestPos );

    HMACsha256(message, messageLen, key, keyLen, digest);
}

static bool verifyDigest( unsigned int digestPos, uint8_t *handshakeMessage, const uint8_t *key, size_t keyLen )
{
    uint8_t calcDigest[SHA256_DIGEST_LENGTH];

    calculateDigest( digestPos, handshakeMessage, key, keyLen, calcDigest );

    return( memcmp( &handshakeMessage[digestPos], calcDigest, SHA256_DIGEST_LENGTH ) == 0 );
}

const uint8_t RtmpConnection::CS0;

int RtmpConnection::smConnectionId = 0;

RtmpConnection::RtmpConnection( Controller *controller, TcpInetSocket *socket ) :
    BinaryConnection( controller, socket ),
    mRtmpController( dynamic_cast<RtmpController *>( controller ) ),
    mTxChunkSize( 128 ),
    mRxChunkSize( 128 ),
    mWindowAckSize( 65536 ),
    mBaseTimestamp( 0 ),
    mServerDigestPosition( 0 ),
    mConnectionId( smConnectionId++ ),
    mState( UNITIALIZED ),
    mFP9( false )
{
    memset( &mC1, 0, sizeof(mC1) );
    memset( &mC2, 0, sizeof(mC2) );
    mC1size = 0;
    memset( &mS1, 0, sizeof(mS1) );
    memset( &mS2, 0, sizeof(mS2) );
    mC2size = 0;

    mRtmpController->newSession( sessionId(), this );
}

RtmpConnection::~RtmpConnection()
{
    mRtmpController->deleteSession( (uintptr_t)this );
}

#if 0
uint32_t RtmpConnection::rtmpTimestamp()
{
    struct timeval now;
    gettimeofday( &now, NULL );
    uint32_t rtmpTime = (now.tv_sec*1000)+(now.tv_usec/1000);

    if ( !mBaseTimestamp )
        mBaseTimestamp = rtmpTime;

    return( rtmpTime-mBaseTimestamp );
}

uint32_t RtmpConnection::rtmpTimestamp24()
{
    return( htonl(rtmpTimestamp()&0xffffff)>>8 );
}

uint32_t RtmpConnection::rtmpTimestamp32()
{
    return( htonl(rtmpTimestamp()) );
}
#else
uint32_t RtmpConnection::rtmpTimestamp( int64_t time )
{
    uint32_t rtmpTime = 0;
    if ( time )
    {
        rtmpTime = time/1000;
    }
    else
    {
        struct timeval now;
        gettimeofday( &now, NULL );
        rtmpTime = (now.tv_sec*1000)+(now.tv_usec/1000);
    }

    if ( !mBaseTimestamp )
        mBaseTimestamp = rtmpTime;

    return( rtmpTime-mBaseTimestamp );
}

uint32_t RtmpConnection::rtmpTimestamp24()
{
    return( 0 );
    return( htobe24(rtmpTimestamp()) );
}

uint32_t RtmpConnection::rtmpTimestamp32()
{
    //return( htobe32(rtmpTimestamp()) );
    static int clk_tck = 0;
    struct tms t;
    if ( !clk_tck )
        clk_tck = sysconf( _SC_CLK_TCK );
    return( times( &t ) * 1000 / clk_tck );
}
#endif

// Transmit an RTMP response message over an established connection
bool RtmpConnection::sendResponse( int chunkStreamId, uint8_t messageTypeId, uint32_t messageStreamId, void *payloadData, size_t payloadSize )
{
    ByteBuffer payload;
    payload.adopt( reinterpret_cast<unsigned char *>(payloadData), payloadSize );
    return( sendResponse( chunkStreamId, messageTypeId, messageStreamId, payload ) );
}

// Header type 0
bool RtmpConnection::sendResponse( int chunkStreamId, uint8_t messageTypeId, uint32_t messageStreamId, ByteBuffer &payload )
{
    ByteBuffer message( 1 );
    message[0] = chunkStreamId;

    ChunkHeader0 header;
    header.timestamp = rtmpTimestamp24();
    header.messageLength = htobe24(payload.size());
    header.messageTypeId = messageTypeId;
    header.messageStreamId = htonl(messageStreamId);
    message.append( &header, sizeof(header) );

    while ( payload.size() )
    {
        size_t chunkSize = min(mTxChunkSize,payload.size());
        message.append( payload.data(), chunkSize );

        payload -= chunkSize;

        if ( payload.size() )
            message.append( 0xc0 | chunkStreamId );
    }
    return( sendResponse( message.data(), message.size() ) );
}

// Header type 1
bool RtmpConnection::sendResponse( int chunkStreamId, uint8_t messageTypeId, ByteBuffer &payload, uint32_t timestampDelta )
{
    ByteBuffer message( 1 );
    message[0] = chunkStreamId;

    ChunkHeader1 header;
    header.timestampDelta = timestampDelta;
    header.messageLength = htobe24(payload.size());
    header.messageTypeId = messageTypeId;
    message.append( &header, sizeof(header) );

    while ( payload.size() )
    {
        size_t chunkSize = min(mTxChunkSize,payload.size());
        message.append( payload.data(), chunkSize );

        payload -= chunkSize;

        if ( payload.size() )
            message.append( 0xc0 | chunkStreamId );
    }
    return( sendResponse( message.data(), message.size() ) );
}

// Header type 2
bool RtmpConnection::sendResponse( int chunkStreamId, ByteBuffer &payload, uint32_t timestampDelta )
{
    ByteBuffer message( 1 );
    message[0] = chunkStreamId;

    ChunkHeader2 header;
    header.timestampDelta = timestampDelta;
    message.append( &header, sizeof(header) );

    while ( payload.size() )
    {
        size_t chunkSize = min(mTxChunkSize,payload.size());
        message.append( payload.data(), chunkSize );

        payload -= chunkSize;

        if ( payload.size() )
            message.append( 0xc0 | chunkStreamId );
    }
    return( sendResponse( message.data(), message.size() ) );
}

bool RtmpConnection::sendResponse( const void *messageData, size_t messageSize )
{
    AutoMutex mutex( mSocketMutex );
    Debug( 2, "Sending %zd byte response", messageSize );
    Hexdump( 3, messageData, min(128,messageSize) );
    if ( mSocket->send( messageData, messageSize ) != messageSize )
    {
        Error( "Unable to send message: %s", strerror(errno) );
        return( false );
    }
    return( true );
}

bool RtmpConnection::recvRequest( ByteBuffer &request )
{
    Debug( 2, "Received RTMP request: (%zd bytes)", request.size() );
    Hexdump( 3, request.data(), min(request.size(),128) );

    if ( mRequest.size() )
    {
        request = mRequest + request;
        mRequest.clear();
    }

    switch( mState )
    {
        case UNITIALIZED :
        {
            // Look for C0 & C1
            if ( request[0] != CS0 )
            {
                Error( "Unexpected value received %02x", request[0] );
                // Close connection
                return( false );
            }
            Debug( 2, "Got RTMP C0 packet" );
            request.consume( sizeof(CS0) );
 
            // Fallthru
        }
        case GOT_C0 :
        {
            if ( request.size() < sizeof(CS1) )
            {
                mRequest = request;
                mState = GOT_C0;
                return( true );
            }

            Debug( 2, "Got RTMP C1 packet" );
            memcpy( &mC1, request.data(), sizeof(mC1) );
            request.consume( sizeof(mC1) );

            if ( mC1.zero & 0xff000000 )
            {
                // FP9 Handshake
                Debug( 2, "Using FP9 handshake" );
                mFP9 = true;
            }

            sendResponse( &CS0, sizeof(CS0) );

            mS1.time = htobe32(rtmpTimestamp32());
            if ( mFP9 )
                mS1.zero = htobe32(0x03050101);
            else
                mS1.zero = 0;
            for ( int i = 0; i < sizeof(mS1.random); i++ )
                mS1.random[i] = 0;
                //mS1.random[i] = rand();

            if ( mFP9 )
            {
                uint8_t *serverSig = (uint8_t *)&mS1;

                mServerDigestPosition = calculateDigestOffset( serverSig, ZM_RTMP_SIG_SIZE, 1 );
                Debug( 3, "Server digest offset: %d", mServerDigestPosition );

                calculateDigest( mServerDigestPosition, serverSig, GenuineFMSKey, 36, serverSig+mServerDigestPosition );

                Debug( 3, "Initial server digest: " );
                Hexdump( 3, serverSig + mServerDigestPosition, SHA256_DIGEST_LENGTH );
            }

            Debug( 2, "Sending RTMP S1 packet" );
            sendResponse( &mS1, sizeof(mS1) );

            if ( mFP9 )
            {
                uint8_t *clientSig = (uint8_t *)&mC1;
                uint8_t digestResp[SHA256_DIGEST_LENGTH];
                uint8_t *signatureResp = NULL;

                /* we have to use this signature now to find the correct algorithms for getting the digest and DH positions */
                int clientDigestPosition = calculateDigestOffset( clientSig, ZM_RTMP_SIG_SIZE, 1 );

                if ( !verifyDigest( clientDigestPosition, clientSig, GenuineFPKey, 30 ) )
                {
                    Debug( 2, "Trying alternate position for client digest" );

                    clientDigestPosition = calculateDigestOffset( clientSig, ZM_RTMP_SIG_SIZE, 2 );

                    if ( !verifyDigest( clientDigestPosition, clientSig, GenuineFPKey, 30 ) )
                    {
                        Error( "Unable to verify client digest" );
                        return false;
                    }
                }

                /* calculate response now */
                signatureResp = clientSig+ZM_RTMP_SIG_SIZE-SHA256_DIGEST_LENGTH;

                HMACsha256( clientSig+clientDigestPosition, SHA256_DIGEST_LENGTH, GenuineFMSKey, sizeof(GenuineFMSKey), digestResp );
                HMACsha256( clientSig, ZM_RTMP_SIG_SIZE - SHA256_DIGEST_LENGTH, digestResp, SHA256_DIGEST_LENGTH, signatureResp );

                /* some info output */
                Debug( 3, "Calculated digest key from secure key and server digest: " );
                Hexdump( 3, digestResp, SHA256_DIGEST_LENGTH );

                Debug( 3, "Server signature calculated:" );
                Hexdump( 3, signatureResp, SHA256_DIGEST_LENGTH );
            }

            Debug( 2, "Sending RTMP S2 packet (reflecting C1 packet)" );
            sendResponse( &mC1, sizeof(mC1) ); // Same as S2
        
            mState = SENT_S2;
            return( true );
            break;
        }
        case SENT_S2 :
        {
            if ( request.size() < sizeof(CS2) )
            {
                mRequest = request;
                return( true );
            }

            Debug( 2, "Got RTMP C2 packet" );
            memcpy( &mC2, request.data(), sizeof(mC2) );
            request.consume( sizeof(mC2) );

            if ( mFP9 )
            {
                uint8_t *serverSig = (uint8_t *)&mS1;
                uint8_t *clientSig = (uint8_t *)&mC2;
                uint8_t signature[SHA256_DIGEST_LENGTH];
                uint8_t digest[SHA256_DIGEST_LENGTH];

                Debug( 3, "Client sent signature:" );
                Hexdump( 3, clientSig+(ZM_RTMP_SIG_SIZE-SHA256_DIGEST_LENGTH), SHA256_DIGEST_LENGTH);

                // Verify client response
                HMACsha256( serverSig+mServerDigestPosition, SHA256_DIGEST_LENGTH, GenuineFPKey, sizeof(GenuineFPKey), digest );
                HMACsha256( clientSig, ZM_RTMP_SIG_SIZE-SHA256_DIGEST_LENGTH, digest, SHA256_DIGEST_LENGTH, signature );

                Debug( 3, "Digest key: " );
                Hexdump( 3, digest, SHA256_DIGEST_LENGTH );

                Debug( 3, "Signature calculated:" );
                Hexdump( 3, signature, SHA256_DIGEST_LENGTH );
                if ( memcmp( signature, clientSig+(ZM_RTMP_SIG_SIZE-SHA256_DIGEST_LENGTH), SHA256_DIGEST_LENGTH ) != 0)
                {
                    Warning( "Client not genuine Adobe!" );
                }
                else
                {
                    Debug( 2, "Client is Genuine Adobe Flash Player" );
                }
            }
            else
            {
                if ( memcmp( &mS1, &mC2, ZM_RTMP_SIG_SIZE ) != 0)
                {
                    Warning( "Client C2 packet does not match S1 packet" );
                }
            }
            Debug( 2, "Handshaking finished...." );

            mState = HANDSHAKE_DONE;
            // Fallthru
        }
        case HANDSHAKE_DONE :
        {
            Debug( 2, "Got %zd bytes", request.size() );
            Hexdump( 3, request.head(), min(1024,request.size()) );

            ByteBuffer response;
            RtmpSession *rtmpSession = mRtmpController->getSession( sessionId() );
            if ( !rtmpSession )
                rtmpSession = mRtmpController->newSession( sessionId(), this );
            if ( !rtmpSession->recvRequest( request, response ) )
                return( false );
            if ( response.size() && !sendResponse( response.data(), response.size() ) )
                return( false );
            return( true );
            break;
        }
    }
    return( false );
}
