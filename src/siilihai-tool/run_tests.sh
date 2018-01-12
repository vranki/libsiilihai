#!/bin/bash
export LD_LIBRARY_PATH=.. 

FORUMID="$(./siilihai-tool list-forums | grep Hacklab | awk '{print $1;}')"
echo Forum is ${FORUMID}

GROUPID="$(./siilihai-tool list-groups --forumid=${FORUMID} | grep Tampere | awk '{print $1;}')"
echo Group is ${GROUPID}

THREADID="$(./siilihai-tool list-threads --forumid=${FORUMID} --group=${GROUPID} | grep Esittele | awk '{print $1;}')"
echo Thread is ${THREADID}

./siilihai-tool list-messages --forumid=${FORUMID} --group=${GROUPID} --thread=${THREADID}

