# PipeConnection

C++ class to make it easier to manage pipe connections.  
The class will automatically create a thread to handle the incoming data.  
Supports 2-way asynchronous communication as well (you will need a separate thread).  

# Example

To create a pipe, use the following:  

```cpp
cPipe* pipe = new cPipe;

if ( !pipe->create( "testpipe" ) )
{
	//handle error
}
```

To connect:  

```cpp
cPipe* pipe = new cPipe;

if ( !pipe->connect( "testpipe" ) )
{
	//handle error
}
```

Everything else you need to know is documented in the files.  
Special thanks to lucass and iPower for helping me with this project.  
Enjoy!
