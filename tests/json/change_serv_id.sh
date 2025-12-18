#!/bin/sh

service_id="$1"

if [ -z "$service_id" ]; then
  echo "Please provide an MBS User Service ID as a parameter or in the mbs_user_service_id shell variable." >&2
  exit 1
fi

scriptdir=`dirname "$0"`
scriptdir=`realpath "$scriptdir"`

for jsonfile in `cd $scriptdir; echo *.json`; do
  case "$jsonfile" in
  *.updated.json)
    ;;
  *)
    sed 's/"mbsUserServId": .*/"mbsUserServId": "'"$service_id"'",/' "$scriptdir/$jsonfile" > "$scriptdir/${jsonfile%.json}.updated.json"
    ;;
  esac
done
