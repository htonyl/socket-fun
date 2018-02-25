#!/bin/bash
PORT=$1
ID=$2

function run {
  CMD=$1
  printf "Run cmd:\n\t$CMD\n\n" && $CMD && printf "\n"
}

function test {
  TESTCASE=$1
  CMD="diff $TESTCASE data/$TESTCASE"
  printf "Run cmd:\n\t$CMD\n" && RESULT=$($CMD)
  if [ -z $RESULT ]; then
    printf "\t$TESTCASE OK!"
  else
    printf "\t$TESTCASE Fails!"
  fi
  echo ""
}

run "./myftpclient 137.189.88.57 $PORT list"
run "./myftpclient 137.189.88.57 $PORT put large1-$ID.tmp"
run "./myftpclient 137.189.88.57 $PORT list"
run "./myftpclient 137.189.88.57 $PORT put large2-$ID.tmp"
run "./myftpclient 137.189.88.57 $PORT list"
run "./myftpclient 137.189.88.57 $PORT get large1-$ID.tmp"
run "./myftpclient 137.189.88.57 $PORT get large2-$ID.tmp"
run "./myftpclient 137.189.88.57 $PORT list"

echo "Check results: "
test "large1-$ID.tmp"
test "large2-$ID.tmp"
echo "DONE!"
