#pragma once
#include <process.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

#define PIPE_READ_ERROR 1
#define PIPE_WRITE_ERROR 2
#define PIPE_CREATE_ERROR 3
#define PIPE_CONNECT_ERROR 4

class pipe_communication
{
public:
	HANDLE get_pipe_handle_server( void );
	HANDLE get_pipe_handle_client( void );
	HANDLE get_thread_handle_server( void );
	HANDLE get_thread_handle_client( void );

	bool create( std::string pipe_name );
	bool connect( const char* pipe_name );
	bool disconnect( void );

	void write( std::string message );
	void read( void );

	bool can_write( void );
	bool can_read( void );

	void clear_write( void );
	void clear_read( void );

	std::string get_message( void );
	void set_message( std::string message );

	std::string get_write_message( void );

	int get_last_error( void );
	void set_last_error( int error );

	bool is_server( void );
private:
	std::string write_buffer;
	std::string read_buffer;

	std::string pipe_name_server = "\\\\.\\pipe\\";
	std::string pipe_name_client = "\\\\.\\pipe\\";

	HANDLE pipe_handle_server = 0;
	HANDLE pipe_handle_client = 0;

	HANDLE thread_handle_server = 0;
	HANDLE thread_handle_client = 0;

	uint32_t thread_id_server;
	uint32_t thread_id_client;

	int last_error = -1;

	bool read_flag = false;
	bool write_flag = false;
	bool created_pipe = false;

	static UINT32 __stdcall server_thread( void* );
	static UINT32 __stdcall client_thread( void* );
};
