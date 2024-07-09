#pragma once

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

/*
	Steinberg Audio Stream I/O API
	(c) 1997 - 2019, Steinberg Media Technologies GmbH

	ASIO Interface Specification v 2.3

	2005 - Added support for DSD sample data (in cooperation with Sony)
	2012 - Added support for drop out detection
		
	

	basic concept is an i/o synchronous double-buffer scheme:
	
	on bufferSwitch(index == 0), host will read/write:

		after ASIOStart(), the
  read  first input buffer A (index 0)
	|   will be invalid (empty)
	*   ------------------------
	|------------------------|-----------------------|
	|                        |                       |
	|  Input Buffer A (0)    |   Input Buffer B (1)  |
	|                        |                       |
	|------------------------|-----------------------|
	|                        |                       |
	|  Output Buffer A (0)   |   Output Buffer B (1) |
	|                        |                       |
	|------------------------|-----------------------|
	*                        -------------------------
	|                        before calling ASIOStart(),
  write                      host will have filled output
                             buffer B (index 1) already

  *please* take special care of proper statement of input
  and output latencies (see ASIOGetLatencies()), these
  control sequencer sync accuracy

*/

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
	#undef MAC 
	#define PPC 0
	#define WINDOWS 1
	#define SGI 0
	#define SUN 0
	#define LINUX 0
	#define BEOS 0

	#define NATIVE_INT64 0
	#define IEEE754_64FLOAT 1
	
#elif BEOS
	#define MAC 0
	#define PPC 0
	#define WINDOWS 0
	#define PC 0
	#define SGI 0
	#define SUN 0
	#define LINUX 0
		
	#define NATIVE_INT64 0
	#define IEEE754_64FLOAT 1
		
	#ifndef DEBUG
		#define DEBUG 0
		#if DEBUG
		 	void DEBUGGERMESSAGE(char *string);
		#else
		  	#define DEBUGGERMESSAGE(a)
		#endif
	#endif

#elif SGI
	#define MAC 0
	#define PPC 0
	#define WINDOWS 0
	#define PC 0
	#define SUN 0
	#define LINUX 0
	#define BEOS 0
		
	#define NATIVE_INT64 0
	#define IEEE754_64FLOAT 1
		
	#ifndef DEBUG
		#define DEBUG 0
		#if DEBUG
		 	void DEBUGGERMESSAGE(char *string);
		#else
		  	#define DEBUGGERMESSAGE(a)
		#endif
	#endif

#else	// MAC

	#define MAC 1
	#define PPC 1
	#define WINDOWS 0
	#define PC 0
	#define SGI 0
	#define SUN 0
	#define LINUX 0
	#define BEOS 0

	#define NATIVE_INT64 0
	#define IEEE754_64FLOAT 1

	#ifndef DEBUG
		#define DEBUG 0
		#if DEBUG
			void DEBUGGERMESSAGE(char *string);
		#else
			#define DEBUGGERMESSAGE(a)
		#endif
	#endif
#endif

// force 4 byte alignment
#if defined(_MSC_VER) && !defined(__MWERKS__) 
#pragma pack(push,4)
#elif PRAGMA_ALIGN_SUPPORTED
#pragma options align = native
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - -
// Type definitions
//- - - - - - - - - - - - - - - - - - - - - - - - -

// number of samples data type is 64 bit integer
#if NATIVE_INT64
	typedef long long int ASIOSamples;
#else
	typedef struct ASIOSamples {
		unsigned long hi;
		unsigned long lo;
	} ASIOSamples;
#endif

// Timestamp data type is 64 bit integer,
// Time format is Nanoseconds.
#if NATIVE_INT64
	typedef long long int ASIOTimeStamp ;
#else
	typedef struct ASIOTimeStamp {
		unsigned long hi;
		unsigned long lo;
	} ASIOTimeStamp;
#endif

// Samplerates are expressed in IEEE 754 64 bit double float,
// native format as host computer
#if IEEE754_64FLOAT
	typedef double ASIOSampleRate;
#else
	typedef struct ASIOSampleRate {
		char ieee[8];
	} ASIOSampleRate;
#endif

// Boolean values are expressed as long
typedef long ASIOBool;
enum {
	ASIOFalse = 0,
	ASIOTrue = 1
};

// Sample Types are expressed as long
enum ASIOSampleType : long {
	ASIOSTInt16MSB   = 0,
	ASIOSTInt24MSB   = 1,		// used for 20 bits as well
	ASIOSTInt32MSB   = 2,
	ASIOSTFloat32MSB = 3,		// IEEE 754 32 bit float
	ASIOSTFloat64MSB = 4,		// IEEE 754 64 bit double float

	// these are used for 32 bit data buffer, with different alignment of the data inside
	// 32 bit PCI bus systems can be more easily used with these
	ASIOSTInt32MSB16 = 8,		// 32 bit data with 16 bit alignment
	ASIOSTInt32MSB18 = 9,		// 32 bit data with 18 bit alignment
	ASIOSTInt32MSB20 = 10,		// 32 bit data with 20 bit alignment
	ASIOSTInt32MSB24 = 11,		// 32 bit data with 24 bit alignment
	
	ASIOSTInt16LSB   = 16,
	ASIOSTInt24LSB   = 17,		// used for 20 bits as well
	ASIOSTInt32LSB   = 18,
	ASIOSTFloat32LSB = 19,		// IEEE 754 32 bit float, as found on Intel x86 architecture
	ASIOSTFloat64LSB = 20, 		// IEEE 754 64 bit double float, as found on Intel x86 architecture

	// these are used for 32 bit data buffer, with different alignment of the data inside
	// 32 bit PCI bus systems can more easily used with these
	ASIOSTInt32LSB16 = 24,		// 32 bit data with 16 bit alignment
	ASIOSTInt32LSB18 = 25,		// 32 bit data with 18 bit alignment
	ASIOSTInt32LSB20 = 26,		// 32 bit data with 20 bit alignment
	ASIOSTInt32LSB24 = 27,		// 32 bit data with 24 bit alignment

	//	ASIO DSD format.
	ASIOSTDSDInt8LSB1   = 32,		// DSD 1 bit data, 8 samples per byte. First sample in Least significant bit.
	ASIOSTDSDInt8MSB1   = 33,		// DSD 1 bit data, 8 samples per byte. First sample in Most significant bit.
	ASIOSTDSDInt8NER8	= 40,		// DSD 8 bit data, 1 sample per byte. No Endianness required.

	ASIOSTLastEntry
};

/*-----------------------------------------------------------------------------
// DSD operation and buffer layout
// Definition by Steinberg/Sony Oxford.
//
// We have tried to treat DSD as PCM and so keep a consistant structure across
// the ASIO interface.
//
// DSD's sample rate is normally referenced as a multiple of 44.1Khz, so
// the standard sample rate is refered to as 64Fs (or 2.8224Mhz). We looked
// at making a special case for DSD and adding a field to the ASIOFuture that
// would allow the user to select the Over Sampleing Rate (OSR) as a seperate
// entity but decided in the end just to treat it as a simple value of
// 2.8224Mhz and use the standard interface to set it.
//
// The second problem was the "word" size, in PCM the word size is always a
// greater than or equal to 8 bits (a byte). This makes life easy as we can
// then pack the samples into the "natural" size for the machine.
// In DSD the "word" size is 1 bit. This is not a major problem and can easily
// be dealt with if we ensure that we always deal with a multiple of 8 samples.
//
// DSD brings with it another twist to the Endianness religion. How are the
// samples packed into the byte. It would be nice to just say the most significant
// bit is always the first sample, however there would then be a performance hit
// on little endian machines. Looking at how some of the processing goes...
// Little endian machines like the first sample to be in the Least Significant Bit,
//   this is because when you write it to memory the data is in the correct format
//   to be shifted in and out of the words.
// Big endian machine prefer the first sample to be in the Most Significant Bit,
//   again for the same reasion.
//
// And just when things were looking really muddy there is a proposed extension to
// DSD that uses 8 bit word sizes. It does not care what endianness you use.
//
// Switching the driver between DSD and PCM mode
// ASIOFuture allows for extending the ASIO API quite transparently.
// See kAsioSetIoFormat, kAsioGetIoFormat, kAsioCanDoIoFormat
//
//-----------------------------------------------------------------------------*/


//- - - - - - - - - - - - - - - - - - - - - - - - -
// Error codes
//- - - - - - - - - - - - - - - - - - - - - - - - -

typedef long ASIOError;
enum {
	ASE_OK = 0,             // This value will be returned whenever the call succeeded
	ASE_SUCCESS = 0x3f4847a0,	// unique success return value for ASIOFuture calls
	ASE_NotPresent = -1000, // hardware input or output is not present or available
	ASE_HWMalfunction,      // hardware is malfunctioning (can be returned by any ASIO function)
	ASE_InvalidParameter,   // input parameter invalid
	ASE_InvalidMode,        // hardware is in a bad mode or used in a bad mode
	ASE_SPNotAdvancing,     // hardware is not running when sample position is inquired
	ASE_NoClock,            // sample clock or rate cannot be determined or is not present
	ASE_NoMemory            // not enough memory for completing the request
};

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

//- - - - - - - - - - - - - - - - - - - - - - - - -
// Time Info support
//- - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct ASIOTimeCode
{       
	double          speed;                  // speed relation (fraction of nominal speed)
	                                        // optional; set to 0. or 1. if not supported
	ASIOSamples     timeCodeSamples;        // time in samples
	unsigned long   flags;                  // some information flags (see below)
	char future[64];
} ASIOTimeCode;

typedef enum ASIOTimeCodeFlags
{
	kTcValid                = 1,
	kTcRunning              = 1 << 1,
	kTcReverse              = 1 << 2,
	kTcOnspeed              = 1 << 3,
	kTcStill                = 1 << 4,
	
	kTcSpeedValid           = 1 << 8
}  ASIOTimeCodeFlags;

typedef struct AsioTimeInfo
{
	double          speed;                  // absolute speed (1. = nominal)
	ASIOTimeStamp   systemTime;             // system time related to samplePosition, in nanoseconds
	                                        // on mac, must be derived from Microseconds() (not UpTime()!)
	                                        // on windows, must be derived from timeGetTime()
	ASIOSamples     samplePosition;
	ASIOSampleRate  sampleRate;             // current rate
	unsigned long flags;                    // (see below)
	char reserved[12];
} AsioTimeInfo;

typedef enum AsioTimeInfoFlags
{
	kSystemTimeValid        = 1,            // must always be valid
	kSamplePositionValid    = 1 << 1,       // must always be valid
	kSampleRateValid        = 1 << 2,
	kSpeedValid             = 1 << 3,
	
	kSampleRateChanged      = 1 << 4,
	kClockSourceChanged     = 1 << 5
} AsioTimeInfoFlags;

typedef struct ASIOTime                          // both input/output
{
	long reserved[4];                       // must be 0
	struct AsioTimeInfo     timeInfo;       // required
	struct ASIOTimeCode     timeCode;       // optional, evaluated if (timeCode.flags & kTcValid)
} ASIOTime;

typedef struct ASIOCallbacks
{
	void (*bufferSwitch) (long doubleBufferIndex, ASIOBool directProcess);
		// bufferSwitch indicates that both input and output are to be processed.
		// the current buffer half index (0 for A, 1 for B) determines
		// - the output buffer that the host should start to fill. the other buffer
		//   will be passed to output hardware regardless of whether it got filled
		//   in time or not.
		// - the input buffer that is now filled with incoming data. Note that
		//   because of the synchronicity of i/o, the input always has at
		//   least one buffer latency in relation to the output.
		// directProcess suggests to the host whether it should immedeately
		// start processing (directProcess == ASIOTrue), or whether its process
		// should be deferred because the call comes from a very low level
		// (for instance, a high level priority interrupt), and direct processing
		// would cause timing instabilities for the rest of the system. If in doubt,
		// directProcess should be set to ASIOFalse.
		// Note: bufferSwitch may be called at interrupt time for highest efficiency.

	void (*sampleRateDidChange) (ASIOSampleRate sRate);
		// gets called when the AudioStreamIO detects a sample rate change
		// If sample rate is unknown, 0 is passed (for instance, clock loss
		// when externally synchronized).

	long (*asioMessage) (long selector, long value, void* message, double* opt);
		// generic callback for various purposes, see selectors below.
		// note this is only present if the asio version is 2 or higher

	ASIOTime* (*bufferSwitchTimeInfo) (ASIOTime* params, long doubleBufferIndex, ASIOBool directProcess);
		// new callback with time info. makes ASIOGetSamplePosition() and various
		// calls to ASIOGetSampleRate obsolete,
		// and allows for timecode sync etc. to be preferred; will be used if
		// the driver calls asioMessage with selector kAsioSupportsTimeInfo.
} ASIOCallbacks;

// asioMessage selectors
enum
{
	kAsioSelectorSupported = 1,	// selector in <value>, returns 1L if supported,
								// 0 otherwise
    kAsioEngineVersion,			// returns engine (host) asio implementation version,
								// 2 or higher
	kAsioResetRequest,			// request driver reset. if accepted, this
								// will close the driver (ASIO_Exit() ) and
								// re-open it again (ASIO_Init() etc). some
								// drivers need to reconfigure for instance
								// when the sample rate changes, or some basic
								// changes have been made in ASIO_ControlPanel().
								// returns 1L; note the request is merely passed
								// to the application, there is no way to determine
								// if it gets accepted at this time (but it usually
								// will be).
	kAsioBufferSizeChange,		// not yet supported, will currently always return 0L.
								// for now, use kAsioResetRequest instead.
								// once implemented, the new buffer size is expected
								// in <value>, and on success returns 1L
	kAsioResyncRequest,			// the driver went out of sync, such that
								// the timestamp is no longer valid. this
								// is a request to re-start the engine and
								// slave devices (sequencer). returns 1 for ok,
								// 0 if not supported.
	kAsioLatenciesChanged, 		// the drivers latencies have changed. The engine
								// will refetch the latencies.
	kAsioSupportsTimeInfo,		// if host returns true here, it will expect the
								// callback bufferSwitchTimeInfo to be called instead
								// of bufferSwitch
	kAsioSupportsTimeCode,		// 
	kAsioMMCCommand,			// unused - value: number of commands, message points to mmc commands
	kAsioSupportsInputMonitor,	// kAsioSupportsXXX return 1 if host supports this
	kAsioSupportsInputGain,     // unused and undefined
	kAsioSupportsInputMeter,    // unused and undefined
	kAsioSupportsOutputGain,    // unused and undefined
	kAsioSupportsOutputMeter,   // unused and undefined
	kAsioOverload,              // driver detected an overload

	kAsioNumMessageSelectors
};

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

//- - - - - - - - - - - - - - - - - - - - - - - - -
// (De-)Construction
//- - - - - - - - - - - - - - - - - - - - - - - - -

/*typedef struct ASIODriverInfo
{
	long asioVersion;		// currently, 2
	long driverVersion;		// driver specific
	char name[32];
	char errorMessage[124];
	void *sysRef;			// on input: system reference
							// (Windows: application main window handle, Mac & SGI: 0)
} ASIODriverInfo;*/


typedef struct ASIOClockSource
{
	long index;					// as used for ASIOSetClockSource()
	long associatedChannel;		// for instance, S/PDIF or AES/EBU
	long associatedGroup;		// see channel groups (ASIOGetChannelInfo())
	ASIOBool isCurrentSource;	// ASIOTrue if this is the current clock source
	char name[32];				// for user selection
} ASIOClockSource;



typedef struct ASIOChannelInfo
{
	long channel;			// on input, channel index
	ASIOBool isInput;		// on input
	ASIOBool isActive;		// on exit
	long channelGroup;		// dto
	ASIOSampleType type;	// dto
	char name[32];			// dto
} ASIOChannelInfo;


//- - - - - - - - - - - - - - - - - - - - - - - - -
// Buffer preparation
//- - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct ASIOBufferInfo
{
	ASIOBool isInput;			// on input:  ASIOTrue: input, else output
	long channelNum;			// on input:  channel index
	void *buffers[2];			// on output: double buffer addresses
} ASIOBufferInfo;


enum
{
	kAsioEnableTimeCodeRead = 1,	// no arguments
	kAsioDisableTimeCodeRead,		// no arguments
	kAsioSetInputMonitor,			// ASIOInputMonitor* in params
	kAsioTransport,					// ASIOTransportParameters* in params
	kAsioSetInputGain,				// ASIOChannelControls* in params, apply gain
	kAsioGetInputMeter,				// ASIOChannelControls* in params, fill meter
	kAsioSetOutputGain,				// ASIOChannelControls* in params, apply gain
	kAsioGetOutputMeter,			// ASIOChannelControls* in params, fill meter
	kAsioCanInputMonitor,			// no arguments for kAsioCanXXX selectors
	kAsioCanTimeInfo,
	kAsioCanTimeCode,
	kAsioCanTransport,
	kAsioCanInputGain,
	kAsioCanInputMeter,
	kAsioCanOutputGain,
	kAsioCanOutputMeter,
	kAsioOptionalOne,
	
	//	DSD support
	//	The following extensions are required to allow switching
	//	and control of the DSD subsystem.
	kAsioSetIoFormat			= 0x23111961,		/* ASIOIoFormat * in params.			*/
	kAsioGetIoFormat			= 0x23111983,		/* ASIOIoFormat * in params.			*/
	kAsioCanDoIoFormat			= 0x23112004,		/* ASIOIoFormat * in params.			*/
	
	// Extension for drop out detection
	kAsioCanReportOverload			= 0x24042012,	/* return ASE_SUCCESS if driver can detect and report overloads */
	
	kAsioGetInternalBufferSamples	= 0x25042012	/* ASIOInternalBufferInfo * in params. Deliver size of driver internal buffering, return ASE_SUCCESS if supported */
};

typedef struct ASIOInputMonitor
{
	long input;		// this input was set to monitor (or off), -1: all
	long output;	// suggested output for monitoring the input (if so)
	long gain;		// suggested gain, ranging 0 - 0x7fffffffL (-inf to +12 dB)
	ASIOBool state;	// ASIOTrue => on, ASIOFalse => off
	long pan;		// suggested pan, 0 => all left, 0x7fffffff => right
} ASIOInputMonitor;

typedef struct ASIOChannelControls
{
	long channel;			// on input, channel index
	ASIOBool isInput;		// on input
	long gain;				// on input,  ranges 0 thru 0x7fffffff
	long meter;				// on return, ranges 0 thru 0x7fffffff
	char future[32];
} ASIOChannelControls;

typedef struct ASIOTransportParameters
{
	long command;		// see enum below
	ASIOSamples samplePosition;
	long track;
	long trackSwitches[16];		// 512 tracks on/off
	char future[64];
} ASIOTransportParameters;

enum
{
	kTransStart = 1,
	kTransStop,
	kTransLocate,		// to samplePosition
	kTransPunchIn,
	kTransPunchOut,
	kTransArmOn,		// track
	kTransArmOff,		// track
	kTransMonitorOn,	// track
	kTransMonitorOff,	// track
	kTransArm,			// trackSwitches
	kTransMonitor		// trackSwitches
};

/*
// DSD support
//	Some notes on how to use ASIOIoFormatType.
//
//	The caller will fill the format with the request types.
//	If the board can do the request then it will leave the
//	values unchanged. If the board does not support the
//	request then it will change that entry to Invalid (-1)
//
//	So to request DSD then
//
//	ASIOIoFormat NeedThis={kASIODSDFormat};
//
//	if(ASE_SUCCESS != ASIOFuture(kAsioSetIoFormat,&NeedThis) ){
//		// If the board did not accept one of the parameters then the
//		// whole call will fail and the failing parameter will
//		// have had its value changes to -1.
//	}
//
// Note: Switching between the formats need to be done before the "prepared"
// state (see ASIO 2 documentation) is entered.
*/
typedef long int ASIOIoFormatType;
enum ASIOIoFormatType_e
{
	kASIOFormatInvalid = -1,
	kASIOPCMFormat = 0,
	kASIODSDFormat = 1,
};

typedef struct ASIOIoFormat_s
{
	ASIOIoFormatType	FormatType;
	char				future[512-sizeof(ASIOIoFormatType)];
} ASIOIoFormat;

// Extension for drop detection
// Note: Refers to buffering that goes beyond the double buffer e.g. used by USB driver designs
typedef struct ASIOInternalBufferInfo
{
	long inputSamples;			// size of driver's internal input buffering which is included in getLatencies
	long outputSamples;			// size of driver's internal output buffering which is included in getLatencies
} ASIOInternalBufferInfo;



typedef interface IAsioDriver IAsioDriver;
interface IAsioDriver : public IUnknown
{
	virtual ASIOBool init(void *sysHandle) = 0;
	virtual void getDriverName(char *name) = 0;
	virtual long getDriverVersion() = 0;
	virtual void getErrorMessage(char *string) = 0; // max 128 including \0
	virtual ASIOError start() = 0;
	virtual ASIOError stop() = 0;
	virtual ASIOError getChannels(long *numInputChannels, long *numOutputChannels) = 0;
	virtual ASIOError getLatencies(long *inputLatency, long *outputLatency) = 0;
	virtual ASIOError getBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity) = 0;
	virtual ASIOError canSampleRate(ASIOSampleRate sampleRate) = 0;
	virtual ASIOError getSampleRate(ASIOSampleRate *sampleRate) = 0;
	virtual ASIOError setSampleRate(ASIOSampleRate sampleRate) = 0;
	virtual ASIOError getClockSources(ASIOClockSource *clocks, long *numSources) = 0;
	virtual ASIOError setClockSource(long reference) = 0;
	virtual ASIOError getSamplePosition(ASIOSamples *sPos, ASIOTimeStamp *tStamp) = 0;
	virtual ASIOError getChannelInfo(ASIOChannelInfo *info) = 0;
	virtual ASIOError createBuffers(ASIOBufferInfo *bufferInfos, long numChannels, long bufferSize, ASIOCallbacks *callbacks) = 0;
	virtual ASIOError disposeBuffers() = 0;
	virtual ASIOError controlPanel() = 0;
	virtual ASIOError future(long selector, void *opt) = 0;
	virtual ASIOError outputReady() = 0;
};

// restore old alignment
#if defined(_MSC_VER) && !defined(__MWERKS__) 
#pragma pack(pop)
#elif PRAGMA_ALIGN_SUPPORTED
#pragma options align = reset
#endif

