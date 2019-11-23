#include "framework.h"

bool cPipe::create( std::string pipe_name )
{
	//Checks if the pipe is already created/connected
	if ( pipe_handle != INVALID_HANDLE_VALUE )
		return false;

	//Truncates the chosen name to the relative path
	pipe_final_name.append( pipe_name );

	//Creates the named pipe
	pipe_handle = CreateNamedPipeA( pipe_final_name.c_str( ) ,	//Pipe name
		PIPE_ACCESS_DUPLEX ,					//Pipe access
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT ,	//Type of the messages
		PIPE_UNLIMITED_INSTANCES ,				//Accepts multiple instances
		BUFSIZE ,						//Output buffer size
		BUFSIZE ,						//Input buffer size
		NMPWAIT_USE_DEFAULT_WAIT ,				//Connection time-out
		NULL );							//Default security attributes

	if ( pipe_handle == INVALID_HANDLE_VALUE ) //If it fails to create the pipe, return false
		return false;

	ov.hEvent = CreateEvent( NULL , false , true , NULL );
	ov.Offset = 0;
	ov.OffsetHigh = 0;

	//Waits for client connection before creating the thread
	bool connected = ConnectNamedPipe( pipe_handle , &ov ) ? TRUE : ( GetLastError( ) == ERROR_PIPE_CONNECTED );

	if ( !connected ) //If there's no connection, close pipe and return false
	{
		CloseHandle( pipe_handle );
		return false;
	}

	thread_handle = CreateThread(
		NULL ,				// No security attribute 
		0 ,				// Default stack size 
		pipe_thread ,			// Thread function
		ReCa < LPVOID >( this ) ,	// Thread parameter 
		0 ,				// Not suspended 
		&thread_id );			// Returns thread ID

	if ( thread_handle == 0 ) //If CreateThread fails, disconnect pipe and return false
	{
		CloseHandle( pipe_handle );
		return false;
	}

	created_pipe = true; //Sets the flag that the pipe has been created successfuly

	return true;
}

bool cPipe::connect( std::string pipe_name )
{
	//Checks if the pipe is already created/connected
	if ( pipe_handle != INVALID_HANDLE_VALUE )
		return false;

	//Truncates the chosen name to the relative path
	pipe_final_name.append( pipe_name );

	//Connects to the pipe
	pipe_handle = CreateFile( pipe_final_name.c_str( ) ,
		GENERIC_WRITE | GENERIC_READ ,
		FILE_SHARE_READ | FILE_SHARE_WRITE ,
		0 ,
		OPEN_EXISTING ,
		FILE_FLAG_OVERLAPPED ,
		0 );

	if ( pipe_handle == INVALID_HANDLE_VALUE ) //If it fails to create the pipe, return false
		return false;

	thread_handle = CreateThread(
		NULL ,				// No security attribute 
		0 ,				// Default stack size 
		pipe_thread ,			// Thread function
		ReCa < LPVOID >( this ) ,	// Thread parameter 
		0 ,				// Not suspended 
		&thread_id );			// Returns thread ID

	if ( thread_handle == 0 ) //If CreateThread fails, disconnect pipe and return false
	{
		CloseHandle( pipe_handle );
		return false;
	}

	ov.hEvent = CreateEvent( NULL , false , true , NULL );
	ov.Offset = 0;
	ov.OffsetHigh = 0;

	return true;
}

bool cPipe::disconnect( void )
{
	if ( created_pipe ) //If the instance created a named pipe, it has to disconnect
	{
		TerminateThread( thread_handle , 0 );
		return DisconnectNamedPipe( pipe_handle ) && CloseHandle( pipe_handle );
	}

	TerminateThread( thread_handle , 0 );
	return CloseHandle( pipe_handle );
}

int cPipe::get_last_error( void )
{
	int last = last_error;			//Gets the value from the last_error to a holder
	last_error = PIPE_NO_ERROR;		//Sets the holder to NO_ERROR
	return last;
}

void cPipe::write( std::string message )
{
	write_buffer = message;			//Sets the message to the buffer
	write_flag = true;			//Sets the flag of writing
	finished_writing = false;

	while ( !finished_writing && last_error == PIPE_NO_ERROR ) //Wait until operation is finished or error
	{
		Sleep( 1 );
	}
}

void cPipe::read( void )
{
	read_flag = true;				   //Sets the flag of reading

	while ( !has_text && last_error == PIPE_NO_ERROR ) //Wait until there's text or error
	{
		Sleep( 1 );
	}
}

std::string cPipe::get_message( void )
{
	char* holder = new char [ read_buffer.length( ) ];
	has_text = false;					//Sets the flag for exiting read() loop
	strcpy( holder , read_buffer.c_str( ) );		//Gets the text from the holder
	read_buffer = "";					//Sets the holder to an empty string
	return holder;
}

HANDLE cPipe::get_pipe_handle( void )
{
	return pipe_handle;
}

HANDLE cPipe::get_thread_handle( void )
{
	return thread_handle;
}

bool cPipe::can_write( void )
{
	return write_flag;
}

bool cPipe::can_read( void )
{
	return read_flag;
}

void cPipe::clear_write( void )
{
	finished_writing = true;
	write_flag = false;
}

void cPipe::clear_read( void )
{
	read_flag = false;
}

void cPipe::set_message( std::string message )
{
	has_text = true;
	read_buffer = message;
}

std::string cPipe::get_write_message( void )
{
	return write_buffer;
}

void cPipe::set_last_error( int error )
{
	last_error = error;
}

DWORD WINAPI cPipe::pipe_thread( LPVOID param )
{
	cPipe* pipe = ReCa < cPipe* >( param );

	bool is_done_reading = false;
	BYTE* out_message = new BYTE [ 2048 ];
	DWORD bytes_read;
	DWORD written;

	while ( true )
	{
		if ( pipe->can_read( ) )
		{
			do
			{
				is_done_reading = ReadFile( pipe->get_pipe_handle( ) , out_message , 1024 * sizeof( TCHAR ) , &bytes_read , &pipe->ov );

				if ( !is_done_reading && GetLastError( ) != ERROR_MORE_DATA && GetLastError( ) != ERROR_IO_PENDING )
				{
					pipe->set_last_error( PIPE_READ_ERROR );
				}

				if ( GetLastError( ) == ERROR_IO_PENDING )
				{
					while ( !HasOverlappedIoCompleted( &pipe->ov ) )
					{
						Sleep( 1 );
					}
					continue;
				}

				is_done_reading = GetOverlappedResult(
					pipe->get_pipe_handle( ) , // handle to pipe 
					&pipe->ov , // OVERLAPPED structure 
					&bytes_read ,            // bytes transferred 
					false );            // do not wait 

				if ( !is_done_reading && GetLastError( ) != ERROR_IO_PENDING )
				{
					pipe->set_last_error( PIPE_READ_ERROR );
				}

			} while ( !is_done_reading );

			pipe->set_message( ReCa < char* >( out_message ) );
			pipe->clear_read( );
		}

		if ( pipe->can_write( ) )
		{
			std::string msg = pipe->get_write_message( );

			if ( !WriteFile( pipe->get_pipe_handle( ) , msg.c_str( ) , msg.length( ) , &written , &pipe->ov ) )
			{
				pipe->set_last_error( PIPE_WRITE_ERROR );
			}

			while ( !GetOverlappedResult(
				pipe->get_pipe_handle( ) , 
				&pipe->ov , 
				&bytes_read ,         
				false ) )          
			{
			}

			pipe->clear_write( );
		}
	}

	return 0;
}
