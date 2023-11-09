#include "stdafx.h"
#include "Packet.h"
#include "DynamicMemoryPool.h"

START_NAMESPACE

CDynamicMemoryPool* CPacket::s_pDynamicMemoryPool = NULL;

void CPacket::InitSendPacketBufferDynamicMemoryPool()
{
	s_pDynamicMemoryPool = new CDynamicMemoryPool( 2, 5000, MAX_PACKET_SIZE, FALSE );
}

void CPacket::ReleaseSendPacketBufferDynamicMemoryPool()
{
	SAFE_DELETE( s_pDynamicMemoryPool );
}

BYTE* CPacket::AllocBuffer()
{
	return reinterpret_cast<BYTE*>( s_pDynamicMemoryPool->Alloc() );
}

void CPacket::FreeBuffer( BYTE* pTarget )
{
	s_pDynamicMemoryPool->Free( reinterpret_cast<char*>(pTarget) );
}

CPacket::CPacket( WORD id, WORD size )
{
	m_PacketBufferSize = size;

	if ( size )
	{
		m_pBuffer = CPacket::AllocBuffer();
		assert( m_pBuffer );

		Initialize();

		*m_header.m_pID = (WORD)id;
	}
	else
	{
		m_pBuffer = NULL;
	}
}

CPacket::~CPacket()
{
	Release();
}

void CPacket::Initialize()
{
	ZeroMemory( m_pBuffer, m_PacketBufferSize );
	m_bEncrypted = false;

	InitBufferPosition();

	*m_header.m_pSize = (WORD)PACKET_HEADER_SIZE;
}

void CPacket::InitBufferPosition()
{
	m_header.m_pSize = (LPWORD)&m_pBuffer[0];
	m_header.m_pID = (LPWORD)&m_pBuffer[2];

	m_pCurrentPositionOfBuffer = m_pBuffer + PACKET_HEADER_SIZE;
}

// 전송버퍼를 size 만큼 재할당한다. 재할당시 이전의 Buffer-Size 가 재할당 size 보다 크면 물리적 메모리 재할당은 일어나지 않는다.
void CPacket::InitPacketBuffer( WORD size )
{
	if ( size > m_PacketBufferSize )
	{
		BYTE* pOldBuffer = m_pBuffer;
		int oldPacketSize = m_PacketBufferSize;

		m_PacketBufferSize = size;

		m_pBuffer = CPacket::AllocBuffer();
		assert( m_pBuffer );
		ZeroMemory( m_pBuffer, m_PacketBufferSize );

		if ( pOldBuffer )
		{
			memcpy( m_pBuffer, pOldBuffer, oldPacketSize );
			CPacket::FreeBuffer( pOldBuffer );			
		}
	}

	InitBufferPosition();
}

void CPacket::Release()
{
	if ( m_pBuffer )
	{
		CPacket::FreeBuffer( m_pBuffer );
		m_pBuffer = NULL;
	}
}

bool CPacket::Encrypt()
{
	if ( !m_bEncrypted )
	{
		WORD nBufferSize = *m_header.m_pSize;
		LPBYTE pData = m_pBuffer + 4;
		int nIndex = 0;

		for ( DWORD i = 0; i < nBufferSize; i++ )
		{
			pData[i] = pData[i] ^ *(m_header.m_pSize + nIndex);
			nIndex = nIndex ^ 1;
		}
	}

	return true;
}

void CPacket::Decrypt()
{
	WORD nBufferSize = *m_header.m_pSize;
	LPBYTE pData = m_pBuffer + 4;
	int nIndex = 0;

	for ( DWORD i = 0; i < nBufferSize; i++ )
	{
		pData[i] = pData[i] ^ *(m_header.m_pSize + nIndex);
		nIndex = nIndex ^ 1;
	}
}

bool CPacket::ReadData( void *data, size_t size )
{
	if ( m_pCurrentPositionOfBuffer + size > m_pBuffer + ( GetSize() -  PACKET_TAIL_SIZE ) )
	{
		return false;
	}

	memcpy( data, m_pCurrentPositionOfBuffer, size);
	m_pCurrentPositionOfBuffer += size;

	return true;
}

void CPacket::WriteData( void *data, size_t size )
{
	assert( GetSize() + size <= MAX_PACKET_SIZE );

	if ( m_pCurrentPositionOfBuffer + ( size + PACKET_DUMMY_SIZE ) >= m_pBuffer + m_PacketBufferSize )
	{
		WORD packetBufferSize = ( m_PacketBufferSize + (WORD)size + PACKET_BUFFER_SIZE );

		InitPacketBuffer( packetBufferSize );

		m_pCurrentPositionOfBuffer = m_pBuffer + ( *( m_header.m_pSize ) );
	}

	memcpy( m_pCurrentPositionOfBuffer, data, size );
	m_pCurrentPositionOfBuffer += size;

	size_t writeSize = (WORD)( *( m_header.m_pSize ) );
	writeSize += size;

	*m_header.m_pSize = (WORD)writeSize;
}

// stream 형태로 전송받은 Buffer 를 data에 size 만큼 복사한다. stream 으로 전송된 데이타는 앞에 2 Byte의 size가 붙어서 온다.
bool CPacket::ReadStream( void *pData , WORD &size )
{
	if ( ReadData( &size, sizeof(WORD) ) == false )
	{
		return ( false );
	}

	return ReadData( pData, size );
}

// stream 형태로 전송할 data 를 전송 Buffer 에 size 만큼 복사한다.
void CPacket::WriteStream( void *pData, WORD size )
{
	WriteData( &size, sizeof(WORD) );
	WriteData( pData, size );
}

// 전송이 완료된 전송 버퍼가 CRC 체크등으로 유효한지 검사한다.
bool CPacket::IsValidate()
{
	WORD* pCheck = (WORD*)( m_pBuffer + ( *( m_header.m_pSize ) ) );

	if ( Checksum() == *pCheck )
	{
		return true;
	}

	return false;
}

WORD CPacket::Checksum()
{
	int size = *( m_header.m_pSize );
	long sum = 0;
	WORD* pBuffer = (WORD*)( (char*)GetBuffer() + PACKET_HEADER_SIZE );
	size -= PACKET_HEADER_SIZE;

	while ( size > 1 )
	{
		sum	+= *pBuffer++;
		size -= 2;
	}

	if ( size == 1 )
		sum += *(BYTE*)pBuffer;

	sum	= ( sum >> 16 ) + ( sum & 0xffff );
	sum	+= ( sum >> 16 );

	return (WORD)~sum;
}

// checksum을 Packet-Buffer의 제일 뒤에 추가한다.
void CPacket::AddCheckSum()
{
	WORD* pCheck = (WORD*)( m_pBuffer + ( ( *( m_header.m_pSize ) )  ) );
	*pCheck = Checksum();
}

CPacket& CPacket::ID( WORD id )
{
	*( m_header.m_pID ) = (WORD)id;

	return *this;
}

WORD CPacket::ID()
{
	if ( ( m_header.m_pID ) == NULL )
	{
		return ( 0 );
	}

	return (WORD) *( m_header.m_pID ) ;
}

WORD CPacket::GetSize()
{
	if ( ( m_header.m_pSize ) == NULL )
	{
		return 0;
	}

	return (WORD)( ( *( m_header.m_pSize ) ) + PACKET_TAIL_SIZE ); 
}

BYTE* CPacket::GetBuffer()
{
	return m_pBuffer; 
}

// 해당 패킷의 전송 Buffer 를 외부의 buffer로 강제 셋팅한다. Receive 의 경우 Session 내의 Receive Buffer 를 사용하므로 Packet의 Buffer를 사용할 필요가 없다.
void CPacket::SetBuffer( BYTE* pBuffer )
{
	m_pBuffer = pBuffer;

	if ( m_pBuffer ) 
		InitPacketBuffer();
	else
		ZeroMemory( &m_header, sizeof( PacketHeader ) );	
}

// 해당 패킷의 전송 Buffer 에 buffer의 값을 size 만큼 복사한다.
void CPacket::SetBuffer( BYTE* pBuffer , WORD size )
{
	InitPacketBuffer( size );
	memcpy( m_pBuffer, pBuffer, size );
}

CPacket& CPacket::operator << ( const char* pIn )
{
	if ( pIn == NULL ) 
		return *this;

	if ( strlen ( pIn ) >= STRING_PACKET_SIZE )
	{
		return *this;
	}

	WriteData( ( void * )pIn, strlen( pIn ) + 1 );

	return *this;
}

CPacket& CPacket::operator << ( char* pIn )
{
	if ( pIn == NULL )
		return *this;

	if ( strlen ( pIn ) >= STRING_PACKET_SIZE )
	{
		return *this;
	}

	WriteData( ( void * )pIn, strlen( pIn ) + 1 );

	return *this;
}

CPacket& CPacket::operator << ( SOCKADDR_IN& in )
{
	*this << in.sin_addr.S_un.S_addr << in.sin_port;
	return *this;
}

CPacket& CPacket::operator >> ( char* pOut )
{
	if ( pOut == NULL ) 
		return *this;

	size_t length = strlen( (const char *)m_pCurrentPositionOfBuffer ) + 1;
	if ( length >= STRING_PACKET_SIZE )
	{
		m_pCurrentPositionOfBuffer += length;
		return ( *this );
	}

	ReadData( pOut, length );

	return *this;
}

CPacket& CPacket::operator >> ( SOCKADDR_IN& out )
{
	*this >> out.sin_addr.S_un.S_addr >> out.sin_port;
	return *this;
}

END_NAMESPACE