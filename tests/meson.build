# License: 5G-MAG Public License (v1.0)
# Author(s): David Waring <david.waring@bbc.co.uk>
# Copyright: (C) 2025 British Broadcasting Corporation
#
# Licensed under the License terms and conditions for use, reproduction, and
# distribution of 5G-MAG software (the “License”).  You may not use this file
# except in compliance with the License.  You may obtain a copy of the License
# at https://www.5g-mag.com/reference-tools.  Unless required by applicable
# law or agreed to in writing, software distributed under the License is
# distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.
#
# See the License for the specific language governing permissions and
# limitations under the License.

libuuid_dep=dependency('uuid', required:true)

#test_object_store_exe = executable('test_ObjectStore', 'test_ObjectStore.cc', test_source_object_store, install:false, include_directories:[libmbsf_libinc, libinc])
#test('test_object_store', test_object_store_exe, verbose: true, timeout: 600, protocol: 'exitcode')

#test_http_server = executable('test_http_server', 'test_http_server.cc', dependencies : [libhttpxpp_dep], install:false)
#test('test_http_server', test_http_server, verbose: true, timeout: 600, protocol: 'exitcode')

# MBSMFMBSSession::setTmgi() AF-supplied-TMGI passthrough, see the test's own file comment.
test_mbsmf_tmgi_exe = executable('test_mbsmf_tmgi', 'test_mbsmf_tmgi.cc',
    include_directories : [libmbsf_libinc, libinc],
    dependencies : [libmbsf_dep],
    install : false)
test('test_mbsmf_tmgi', test_mbsmf_tmgi_exe, suite: 'unit',
     args: ['-c'] + files('test_mbsmf_tmgi.yaml'), timeout: 60, protocol: 'exitcode')

# MultipartMime's own body assembly (RFC 2045/2387/2557 boundary, ordering, filename/Content-
# Location escaping), see the test's own file comment.
test_multipart_mime_exe = executable('test_multipart_mime', 'test_multipart_mime.cc',
    include_directories : [libmbsf_libinc, libinc],
    dependencies : [libmbsf_dep],
    install : false)
test('test_multipart_mime', test_multipart_mime_exe, suite: 'unit', timeout: 60, protocol: 'exitcode')
