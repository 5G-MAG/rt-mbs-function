#!/bin/sh
##############################################################################
# 5G-MAG Reference Tools: MBS Function: OpenAPI-Generator post process
##############################################################################
# Copyright: (C)2024 British Broadcasting Corporation
# Author(s): David Waring <david.waring2@bbc.co.uk>
# License: 5G-MAG Public License v1
#
# Licensed under the License terms and conditions for use, reproduction, and
# distribution of 5G-MAG software (the “License”).  You may not use this file
# except in compliance with the License.  A copy of the license may be found in
# the LICENSE file accompanying this software or you may obtain a copy of the
# License at https://www.5g-mag.com/reference-tools.  Unless required by
# applicable law or agreed to in writing, software distributed under the
# License is distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR CONDITIONS
# OF ANY KIND, either express or implied.
#
# See the License for the specific language governing permissions and
# limitations under the License.
##############################################################################

scriptdir=`dirname "$0"`
scriptdir=`realpath "$scriptdir"`

file="$1"

case "$file" in
*.cc|*.cpp)
        sed -E -i '/StringValidator[^(]*\("[^"]*", nullptr, "[^\/]/ {h;s/.*StringValidator[^(]*\("[^"]*", nullptr, "//;s/([^\\])".*/\1/;s/\\/\\\\/;s/^/\//;s/$/\//;H;x;s/(StringValidator[^(]*\("[^"]*", nullptr, ")[^\n]*("[^\n]*)\n(.*)/\1\3\2/}; /"\/([^"]|\\")*\/"/ {h;s@.*"/@@;s@/".*@@;s@\\/@\\\\/@;s@\\\\\\/@\\\\/@;H;x;s/(.*"\/)([^"]|\\")*(\/"[^\n]*)\n(.*)/\1\4\3/}' "$file"
        ;;
esac
