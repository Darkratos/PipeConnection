#include "framework.h"
#include <process.h>

HANDLE pipe_communication::get_pipe_handle_server( void )
{
	return pipe_handle_server;
}

HANDLE pipe_communication::get_pipe_handle_client( void )
{
	return pipe_handle_client;
}

HANDLE pipe_communication::get_thread_handle_server( void )
{
	return thread_handle_server;
}

HANDLE pipe_communication::get_thread_handle_client( void )
{
	return thread_handle_client;
}

bool pipe_communication::create( std::string pipe_name )
{
	if ( pipe_handle_server != 0 && pipe_handle_client != 0 )
		return false;

	pipe_name_server.append( pipe_name );
	pipe_name_server.append( "_server" );

	pipe_name_client.append( pipe_name );
	pipe_name_client.append( "_client" );

	pipe_handle_server = CreateNamedPipeA( pipe_name_server.c_str( ) ,
		PIPE_ACCESS_INBOUND ,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT ,
		PIPE_UNLIMITED_INSTANCES ,
		1024 * 16 ,
		1024 * 16 ,
		NMPWAIT_USE_DEFAULT_WAIT ,
		NULL );

	pipe_handle_client = CreateNamedPipeA( pipe_name_client.c_str( ) ,
		PIPE_ACCESS_OUTBOUND ,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT ,
		PIPE_UNLIMITED_INSTANCES ,
		1024 * 16 ,
		1024 * 16 ,
		NMPWAIT_USE_DEFAULT_WAIT ,
		NULL );

	if ( pipe_handle_server != INVALID_HANDLE_VALUE && pipe_handle_client != INVALID_HANDLE_VALUE )
	{
		thread_handle_server = reinterpret_cast < HANDLE > ( _beginthreadex( NULL , NULL , server_thread , this , 0 , &thread_id_server ) );
		thread_handle_client = reinterpret_cast < HANDLE > ( _beginthreadex( NULL , NULL , client_thread , this , 0 , &thread_id_client ) );
		created_pipe = true;
		return pipe_handle_server != INVALID_HANDLE_VALUE && pipe_handle_client != INVALID_HANDLE_VALUE && thread_handle_server != INVALID_HANDLE_VALUE && thread_handle_client != INVALID_HANDLE_VALUE;
	}

	return false;
}

bool pipe_communication::connect( const char* pipe_name )
{
	if ( pipe_handle_server != 0 && pipe_handle_client != 0 )
		return false;

	pipe_name_server.append( pipe_name );
	pipe_name_server.append( "_server" );

	pipe_name_client.append( pipe_name );
	pipe_name_client.append( "_client" );

	pipe_handle_server = CreateFile( pipe_name_server.c_str( ) ,
		GENERIC_WRITE ,
		FILE_SHARE_READ | FILE_SHARE_WRITE ,
		0 ,
		OPEN_EXISTING ,
		0 ,
		0 );

	pipe_handle_client = CreateFile( pipe_name_client.c_str( ) ,
		GENERIC_READ ,
		FILE_SHARE_READ | FILE_SHARE_WRITE ,
		0 ,
		OPEN_EXISTING ,
		0 ,
		0 );

	if ( pipe_handle_server != INVALID_HANDLE_VALUE && pipe_handle_client != INVALID_HANDLE_VALUE )
	{
		thread_handle_server = reinterpret_cast < HANDLE > ( _beginthreadex( NULL , NULL , server_thread , this , 0 , &thread_id_server ) );
		thread_handle_client = reinterpret_cast < HANDLE > ( _beginthreadex( NULL , NULL , client_thread , this , 0 , &thread_id_client ) );

		return thread_handle_server != INVALID_HANDLE_VALUE && thread_handle_client != INVALID_HANDLE_VALUE;
	}

	return false;
}

bool pipe_communication::disconnect( void )
{
	if ( created_pipe )
		return DisconnectNamedPipe( pipe_handle_client ) && DisconnectNamedPipe( pipe_handle_server );

	return CloseHandle( pipe_handle_client ) && CloseHandle( pipe_handle_server );
}

void pipe_communication::write( std::string message )
{
	write_buffer = message;
	write_flag = true;
}

void pipe_communication::read( void )
{
	read_flag = true;
}

bool pipe_communication::can_read( void )
{
	return read_flag;
}

bool pipe_communication::can_write( void )
{
	return write_flag;
}

std::string pipe_communication::get_message( void )
{
	std::string holder = read_buffer;
	read_buffer = "";
	return holder;
}

void pipe_communication::set_message( std::string message )
{
	read_buffer = message;
}

int pipe_communication::get_last_error( void )
{
	int last = last_error;
	last_error = -1;
	return last;
}

void pipe_communication::set_last_error( int error )
{
	last_error = error;
}

void pipe_communication::clear_write( void )
{
	write_flag = false;
}

void pipe_communication::clear_read( void )
{
	read_flag = false;
}

std::string pipe_communication::get_write_message( void )
{
	return write_buffer;
}

bool pipe_communication::is_server( void )
{
	return created_pipe;
}

UINT32 __stdcall pipe_communication::server_thread( void* pParam )
{
	pipe_communication* cur_pipe = reinterpret_cast < pipe_communication* > ( pParam );

	if ( cur_pipe->is_server( ) )
		if ( !ConnectNamedPipe( cur_pipe->pipe_handle_server , nullptr ) )
			return false;

	bool is_done_reading = false;
	BYTE* out_message = new BYTE [ 2048 ];
	DWORD bytes_read;

	while ( true )
	{
		if ( cur_pipe->can_read( ) )
		{
			HANDLE target_handle = cur_pipe->is_server( ) ? cur_pipe->get_pipe_handle_server( ) : cur_pipe->get_pipe_handle_client( );

			do
			{
				is_done_reading = ReadFile( target_handle , out_message , 1024 * sizeof( TCHAR ) , &bytes_read , NULL );

				if ( !is_done_reading && GetLastError( ) != ERROR_MORE_DATA )
				{
					cur_pipe->set_last_error( PIPE_READ_ERROR );
				}
			} while ( !is_done_reading );

			cur_pipe->set_message( reinterpret_cast < char* > ( out_message ) );
			cur_pipe->clear_read( );
		}
	}
}

UINT32 __stdcall pipe_communication::client_thread( void* pParam )
{
	pipe_communication* cur_pipe = reinterpret_cast < pipe_communication* > ( pParam );

	if ( cur_pipe->is_server( ) )
		if ( !ConnectNamedPipe( cur_pipe->pipe_handle_client , nullptr ) )
			return false;

	DWORD written;

	while ( true )
	{
		if ( cur_pipe->can_write( ) )
		{
			std::string msg = cur_pipe->get_write_message( );

			HANDLE target_handle = cur_pipe->is_server( ) ? cur_pipe->get_pipe_handle_client( ) : cur_pipe->get_pipe_handle_server( );

			if ( !WriteFile( target_handle , msg.c_str( ) , msg.length( ) , &written , NULL ) )
			{
				cur_pipe->set_last_error( PIPE_WRITE_ERROR );
			}

			cur_pipe->clear_write( );
		}
	}
}
