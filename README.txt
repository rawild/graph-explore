READ ME for graph-explore.c

To run this program
compile it with make graph-explore in the src directory


The run ./graph-explore with the options for
-h host -p port -u uri -a attribute and -v value


The host is the host of the uri.

The port is the port to connect to the external host.

and the uri is the uri of the object you are trying to access.

The attribute is the element you are searching.

The value is the value you are looking for that element to be.


Example:
make graph-explore
./graph-explore -h localhost -p 8888 -u file_one.txt -a Status -v Student