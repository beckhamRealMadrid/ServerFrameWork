#pragma once

START_NAMESPACE

#define MAX_PACKET_SIZE		512
#define	STRING_PACKET_SIZE	200
#define PACKET_BUFFER_SIZE	256
#define	PACKET_DUMMY_SIZE	64
#define	PACKET_HEADER_SIZE	4
#define	PACKET_TAIL_SIZE	2
#define	UDP_HEADER_SIZE		5

class CDynamicMemoryPool;

class RENET_API CPacket
{
private:
	struct PacketHeader
	{
		LPWORD m_pSize;
		LPWORD m_pID;
	};
public:
	CPacket( WORD id = 0 , WORD size = PACKET_BUFFER_SIZE );
	virtual ~CPacket();

	void				Initialize();
	void				Release();
	bool				Encrypt();
	void				Decrypt();
	void				AddCheckSum();
	bool				ReadStream( void *pData , WORD &size );
	void				WriteStream( void *pData, WORD size );
	bool				IsValidate();
	CPacket&			ID( WORD id );
	WORD				ID();
	WORD				GetSize();
	BYTE*				GetBuffer();
	void				SetBuffer( BYTE* pBuffer );
	void				SetBuffer( BYTE* pBuffer , WORD size );

	CPacket& operator << ( const char* pIn );
	CPacket& operator << ( char* pIn );
	CPacket& operator << ( SOCKADDR_IN &in );
	CPacket& operator >> ( char* pOut );
	CPacket& operator >> ( SOCKADDR_IN &out );

	template < class _T >
	CPacket& operator << ( _T in  )
	{
		WriteData( &in, sizeof( _T ) );
		return ( *this );
	}

	template < class _T >
	CPacket& operator >> ( _T &in  )
	{
		ReadData( &in, sizeof( _T ) );
		return ( *this );
	}	

protected:
	bool				ReadData( void *data, size_t size );
	void				WriteData( void *data, size_t size );
	WORD				Checksum();

private:
	void				InitBufferPosition();
	void				InitPacketBuffer( WORD size = 0 );

protected:
	PacketHeader		m_header;
	WORD				m_PacketBufferSize;
	BYTE*				m_pBuffer;
	BYTE*				m_pCurrentPositionOfBuffer;	
	bool				m_bEncrypted;

	static CDynamicMemoryPool*	s_pDynamicMemoryPool;
public:
	static void			InitSendPacketBufferDynamicMemoryPool();
	static void			ReleaseSendPacketBufferDynamicMemoryPool();
	static BYTE*		AllocBuffer();
	static void			FreeBuffer( BYTE* pTarget );
};

END_NAMESPACE