# License: 5G-MAG Public License (v1.0)
# Author(s): David Waring <david.waring2@bbc.co.uk>
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

libapp_dep = open5gs_project.get_variable('libapp_dep')
libcrypt_dep = open5gs_project.get_variable('libcrypt_dep')
libsbi_dep = open5gs_project.get_variable('libsbi_dep')
libsbi_openapi_dep = open5gs_project.get_variable('libsbi_openapi_dep')
open5gs_sysconfdir = open5gs_project.get_variable('open5gs_sysconfdir')
srcinc = open5gs_project.get_variable('srcinc')
libdir = open5gs_project.get_variable('libdir')
python3_exe = open5gs_project.get_variable('python3_exe')
mkdir_p = open5gs_project.get_variable('mkdir_p')
install_conf = open5gs_project.get_variable('install_conf')
libsbi_openapi_inc = open5gs_project.get_variable('libsbi_openapi_inc')
libinc = open5gs_project.get_variable('libinc')

libscmbsmf_dep = svc_consumers_project.get_variable('libscmbsmf_dep')

libhttpxpp_dep = common_shared_project.get_variable('libhttpxpp_dep')
librtsdp_dep = common_shared_project.get_variable('librtsdp_dep')

app_type = 'MBSF'

latest_apis = get_option('latest_apis')
fiveg_api_approval = get_option('fiveg_api_approval')
fiveg_api_release = get_option('fiveg_api_release')

fs = import('fs')

uuid_dep = dependency('uuid', required: true)

#test_source_object_store = test_source_subscriber_subscription + files('''
#  ObjectStore.cc
#  ObjectStore.hh
#  '''.split())

libmbsf_dist_sources = files('''
    AlwaysActive.cc
    AlwaysActive.hh
    ActivePeriods.cc
    ActivePeriods.hh
    ActivePeriodsBase.hh
    ActivePeriodsRepRule.cc
    ActivePeriodsRepRule.hh
    ApplicationServiceDesc.cc
    ApplicationServiceDesc.hh
    AllocationRetentionPriority.cc
    AllocationRetentionPriority.hh
    AnnouncementBundleIndexHandler.cc
    AnnouncementBundleIndexHandler.hh
    AssociatedSessId.cc
    AssociatedSessId.hh
    AvailabilityInfo.cc
    AvailabilityInfo.hh
    common.cc
    common.hh
    CarouselObject.cc
    CarouselObject.hh
    CaseInsensitiveTraits.hh
    CivicAddr.cc
    CivicAddr.hh
    Context.cc
    Context.hh
    Curl.cc
    Curl.hh
    DistributionSessionInfo.cc
    DistributionSessionInfo.hh
    DistributionSessionDesc.cc
    DistributionSessionDesc.hh
    DistributionSessionInfoSubscription.cc
    DistributionSessionInfoSubscription.hh
    DistributionSessionNotificationEvent.cc
    DistributionSessionNotificationEvent.hh
    EventHandler.hh
    ExternalServiceArea.cc
    ExternalServiceArea.hh
    GeographicalArea.cc
    GeographicalArea.hh
    GeographicalCoordinate.cc
    GeographicalCoordinate.hh
    hash.hh
    LocalEvents.hh
    MBSFEventHandler.cc
    MBSFEventHandler.hh
    MBSFNetworkFunction.hh
    MBSMFMBSSession.cc
    MBSMFMBSSession.hh
    MBSNcgi.cc
    MBSNcgi.hh
    MBSNcgiTai.cc
    MBSNcgiTai.hh
    MBSPlmnId.cc
    MBSPlmnId.hh
    MBSProblemCause.cc
    MBSProblemCause.hh
    MediaComp.cc
    MediaComp.hh
    MediaInfo.cc
    MediaInfo.hh
    MultipartMime.cc
    MultipartMime.hh
    NfServer.cc
    NfServer.hh
    Nmb2Build.cc
    Nmb2Build.hh
    Nmb2Handler.cc
    Nmb2Handler.hh
    ObjManifest.cc
    ObjManifest.hh
    ObjRepairParameters.cc
    ObjRepairParameters.hh
    Open5GSEvent.hh
    Open5GSFSM.hh
    Open5GSNetworkFunction.cc
    Open5GSNetworkFunction.hh
    Open5GSSBIClient.cc
    Open5GSSBIClient.hh
    Open5GSSBIHeader.cc
    Open5GSSBIHeader.hh
    Open5GSSBIObject.cc
    Open5GSSBIObject.hh
    Open5GSSBIMessage.cc
    Open5GSSBIMessage.hh
    Open5GSSBINFInstance.cc
    Open5GSSBINFInstance.hh
    Open5GSSBIRequest.cc
    Open5GSSBIRequest.hh
    Open5GSSBIResponse.cc
    Open5GSSBIResponse.hh
    Open5GSSBIServer.cc
    Open5GSSBIServer.hh
    Open5GSSBIStream.hh
    Open5GSSockAddr.hh
    Open5GSTimer.hh
    Open5GSYamlDocument.hh
    Open5GSYamlIter.cc
    Open5GSYamlIter.hh
    QoSReq.cc
    QoSReq.hh
    ServiceArea.cc
    ServiceArea.hh
    ServiceInfo.cc
    ServiceInfo.hh
    SubscribedEvents.cc
    SubscribedEvents.hh
    ServiceScheduleDesc.cc
    ServiceScheduleDesc.hh
    TimerFunc.hh
    TrackingAreaIdentity.cc
    TrackingAreaIdentity.hh
    UniqueMBSSessionId.cc
    UniqueMBSSessionId.hh
    UserDataIngSession.cc
    UserDataIngSession.hh
    UserDataIngSessionNotificationEvent.cc
    UserDataIngSessionNotificationEvent.hh
    UserDataIngStatSubsc.cc
    UserDataIngStatSubsc.hh
    UserService.cc
    UserService.hh
    UserServiceAnnBundle.cc
    UserServiceAnnBundle.hh
    UserServiceAnnChannel.cc
    UserServiceAnnChannel.hh
    UserServiceDesc.cc
    UserServiceDesc.hh
    utilities.cc
    utilities.hh
'''.split())

api_tag = latest_apis?'REL-'+fiveg_api_release:'TSG'+fiveg_api_approval+'-Rel'+fiveg_api_release

gen_5gmbsf_sh = find_program('sh')
openapi_dep_file = meson.current_source_dir() / '.openapi.srcs'
message('Generating OpenAPI bindings for version '+api_tag+' of the 5G APIs...')
openapi_gen_result = run_command([gen_5gmbsf_sh,'-c','"$MESON_SOURCE_ROOT/$MESON_SUBDIR/generator-mbsf" -M "'+openapi_dep_file+'" -b '+api_tag], capture: true, check: true)
libmbsf_openapi_gen_sources = files(fs.read(openapi_dep_file).split())

version_conf = configuration_data()
version_conf.set_quoted('MBSF_NAME', meson.project_name())
version_conf.set_quoted('MBSF_VERSION', meson.project_version())
version_conf.set_quoted('MBSF_TYPE', app_type)
version_conf.set_quoted('FIVEG_API_RELEASE', fiveg_api_release)

libmbsf_gen_sources = [configure_file(output : 'mbsf-version.h', configuration : version_conf)]

libmbsf_sources = libmbsf_dist_sources + libmbsf_gen_sources + libmbsf_openapi_gen_sources

libmbsf_libinc = include_directories('.')

libmbsf = static_library('mbsf',
    sources : libmbsf_sources,
    include_directories : [libmbsf_libinc, libsbi_openapi_inc, libinc],
    dependencies : [libsbi_dep,
                    libsbi_openapi_dep,
                    libapp_dep,
                    libcrypt_dep,
                    libscmbsmf_dep,
                    uuid_dep,
                    libhttpxpp_dep,
                    librtsdp_dep],
    install : false)

libmbsf_dep = declare_dependency(
    link_with : libmbsf,
    dependencies : [libsbi_dep,
                    libsbi_openapi_dep,
                    libapp_dep,
                    libcrypt_dep,
                    libscmbsmf_dep,
                    uuid_dep,
                    libhttpxpp_dep,
                    librtsdp_dep])
libmbsf_whole_dep = declare_dependency(
    link_whole : libmbsf,
    dependencies : [libsbi_dep,
                    libsbi_openapi_dep,
                    libapp_dep,
                    libcrypt_dep,
                    libscmbsmf_dep,
                    uuid_dep,
                    libhttpxpp_dep,
                    librtsdp_dep])

mbsf_sources = files('''
    App.cc
    App.hh
'''.split()) + open5gs_project.get_variable('app_main_c')

mbsf_config_source = '''
    mbsf.yaml
'''.split()

mbsf_include = include_directories('.')

executable('open5gs-mbsfd',
    sources : mbsf_sources,
    c_args : '-DDEFAULT_CONFIG_FILENAME="@0@/mbsf.yaml"'.format(open5gs_sysconfdir),
    cpp_args : '-DDEFAULT_CONFIG_FILENAME="@0@/mbsf.yaml"'.format(open5gs_sysconfdir),
    include_directories : [srcinc],
    dependencies : [libmbsf_whole_dep],
    install_rpath : libdir,
    install : true)

meson.add_install_script(python3_exe, '-c', mkdir_p.format(open5gs_sysconfdir))
conf_configuration = configuration_data()
conf_configuration.set('localstatedir', get_option('prefix') / get_option('localstatedir'))
conf_configuration.set('sysconfdir', get_option('prefix') / get_option('sysconfdir'))
foreach conf_file : mbsf_config_source
    gen = configure_file(input : conf_file + '.in', configuration : conf_configuration, output : conf_file)
    meson.add_install_script(python3_exe, '-c', install_conf.format(gen.full_path(), open5gs_sysconfdir))
endforeach
