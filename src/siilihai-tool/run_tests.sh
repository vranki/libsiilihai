#!/bin/bash
export LD_LIBRARY_PATH=.. 
./siilihai-tool list-forums
./siilihai-tool list-groups --forumid=440
./siilihai-tool list-threads --forumid=440 --group=38
./siilihai-tool list-messages --forumid=440 --group=38 --thread=730

