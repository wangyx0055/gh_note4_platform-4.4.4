// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/chrome_browser_main_extra_parts_profiles.h"

#include "apps/app_keep_alive_service_factory.h"
#include "apps/app_load_service_factory.h"
#include "apps/app_restore_service_factory.h"
#include "apps/app_window_geometry_cache.h"
#include "chrome/browser/apps/ephemeral_app_service_factory.h"
#include "chrome/browser/apps/shortcut_manager_factory.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/background/background_contents_service_factory.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/chrome_browser_main.h"
#include "chrome/browser/content_settings/cookie_settings.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/geolocation/chrome_geolocation_permission_context_factory.h"
#include "chrome/browser/google/google_url_tracker_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/history/shortcuts_backend_factory.h"
#include "chrome/browser/invalidation/invalidation_service_factory.h"
#include "chrome/browser/media_galleries/media_galleries_preferences_factory.h"
#include "chrome/browser/notifications/desktop_notification_service_factory.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/plugins/plugin_prefs_factory.h"
#include "chrome/browser/policy/profile_policy_connector_factory.h"
#include "chrome/browser/predictors/autocomplete_action_predictor_factory.h"
#include "chrome/browser/predictors/predictor_database_factory.h"
#if defined(ENABLE_PRERENDER)
#include "chrome/browser/prerender/prerender_link_manager_factory.h"
#include "chrome/browser/prerender/prerender_manager_factory.h"
#endif
#include "chrome/browser/printing/cloud_print/cloud_print_proxy_service_factory.h"
#include "chrome/browser/profiles/gaia_info_update_service_factory.h"
#include "chrome/browser/search/hotword_service_factory.h"
#include "chrome/browser/search/instant_service_factory.h"
#include "chrome/browser/search_engines/template_url_fetcher_factory.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#if defined(ENABLE_SIGNIN)
#include "chrome/browser/signin/about_signin_internals_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#endif
#include "chrome/browser/speech/extension_api/tts_extension_api.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/themes/theme_service_factory.h"
#if defined(ENABLE_MOSTVISITED)
#include "chrome/browser/thumbnails/thumbnail_service_factory.h"
#endif
#include "chrome/browser/ui/find_bar/find_bar_state_factory.h"
#include "chrome/browser/ui/global_error/global_error_service_factory.h"
#include "chrome/browser/ui/tabs/pinned_tab_service_factory.h"
#if defined(ENABLE_WEBUI)        
#include "chrome/browser/ui/webui/ntp/ntp_resource_cache_factory.h"
#endif
#include "chrome/browser/undo/bookmark_undo_service_factory.h"
#include "chrome/browser/webdata/web_data_service_factory.h"

#if defined(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/activity_log/activity_log.h"
#include "chrome/browser/extensions/api/activity_log_private/activity_log_private_api.h"
#include "chrome/browser/extensions/api/alarms/alarm_manager.h"
#include "chrome/browser/extensions/api/api_resource_manager.h"
#include "chrome/browser/extensions/api/audio/audio_api.h"
#include "chrome/browser/extensions/api/bluetooth/bluetooth_api_factory.h"
#include "chrome/browser/extensions/api/bookmarks/bookmarks_api.h"
#include "chrome/browser/extensions/api/braille_display_private/braille_display_private_api.h"
#include "chrome/browser/extensions/api/commands/command_service.h"
#include "chrome/browser/extensions/api/cookies/cookies_api.h"
#include "chrome/browser/extensions/api/developer_private/developer_private_api_factory.h"
#include "chrome/browser/extensions/api/dial/dial_api_factory.h"
#include "chrome/browser/extensions/api/extension_action/extension_action_api.h"
#include "chrome/browser/extensions/api/feedback_private/feedback_private_api.h"
#include "chrome/browser/extensions/api/font_settings/font_settings_api.h"
#include "chrome/browser/extensions/api/history/history_api.h"
#include "chrome/browser/extensions/api/hotword_private/hotword_private_api.h"
#include "chrome/browser/extensions/api/identity/identity_api.h"
#include "chrome/browser/extensions/api/idle/idle_manager_factory.h"
#include "chrome/browser/extensions/api/input/input.h"
#include "chrome/browser/extensions/api/location/location_manager.h"
#include "chrome/browser/extensions/api/management/management_api.h"
#include "chrome/browser/extensions/api/mdns/mdns_api.h"
#include "chrome/browser/extensions/api/media_galleries_private/media_galleries_private_api.h"
#include "chrome/browser/extensions/api/networking_private/networking_private_event_router_factory.h"
#include "chrome/browser/extensions/api/omnibox/omnibox_api.h"
#include "chrome/browser/extensions/api/preference/chrome_direct_setting_api.h"
#include "chrome/browser/extensions/api/preference/preference_api.h"
#include "chrome/browser/extensions/api/processes/processes_api.h"
#include "chrome/browser/extensions/api/push_messaging/push_messaging_api.h"
#include "chrome/browser/extensions/api/runtime/runtime_api_factory.h"
#include "chrome/browser/extensions/api/serial/serial_connection.h"
#include "chrome/browser/extensions/api/sessions/sessions_api.h"
#include "chrome/browser/extensions/api/settings_overrides/settings_overrides_api.h"
#include "chrome/browser/extensions/api/signed_in_devices/signed_in_devices_manager.h"
#include "chrome/browser/extensions/api/socket/socket.h"
#include "chrome/browser/extensions/api/socket/tcp_socket.h"
#include "chrome/browser/extensions/api/socket/udp_socket.h"
#include "chrome/browser/extensions/api/sockets_tcp/tcp_socket_event_dispatcher.h"
#include "chrome/browser/extensions/api/sockets_tcp_server/tcp_server_socket_event_dispatcher.h"
#include "chrome/browser/extensions/api/sockets_udp/udp_socket_event_dispatcher.h"
#include "chrome/browser/extensions/api/streams_private/streams_private_api.h"
#include "chrome/browser/extensions/api/system_info/system_info_api.h"
#include "chrome/browser/extensions/api/tab_capture/tab_capture_registry.h"
#include "chrome/browser/extensions/api/tabs/tabs_windows_api.h"
#include "chrome/browser/extensions/api/usb/usb_device_resource.h"
#include "chrome/browser/extensions/api/web_navigation/web_navigation_api.h"
#include "chrome/browser/extensions/api/web_request/web_request_api.h"
#include "chrome/browser/extensions/api/webrtc_audio_private/webrtc_audio_private_api.h"
#include "chrome/browser/extensions/extension_system_factory.h"
#include "chrome/browser/extensions/extension_toolbar_model_factory.h"
#include "chrome/browser/extensions/extension_web_ui_override_registrar.h"
#include "chrome/browser/extensions/install_tracker_factory.h"
#include "chrome/browser/extensions/menu_manager_factory.h"
#include "chrome/browser/extensions/plugin_manager.h"
#include "chrome/browser/extensions/token_cache/token_cache_service_factory.h"
#include "extensions/browser/extension_prefs_factory.h"
#include "extensions/browser/renderer_startup_helper.h"
#endif  // defined(ENABLE_EXTENSIONS)

#if defined(ENABLE_CAPTIVE_PORTAL_DETECTION)
#include "chrome/browser/captive_portal/captive_portal_service_factory.h"
#endif

#if defined(ENABLE_CONFIGURATION_POLICY)
#include "chrome/browser/policy/cloud/policy_header_service_factory.h"
#include "chrome/browser/policy/cloud/user_cloud_policy_invalidator_factory.h"
#include "chrome/browser/policy/schema_registry_service_factory.h"
#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/policy/policy_cert_service_factory.h"
#include "chrome/browser/chromeos/policy/recommendation_restorer_factory.h"
#include "chrome/browser/chromeos/policy/user_cloud_policy_manager_factory_chromeos.h"
#include "chrome/browser/chromeos/policy/user_cloud_policy_token_forwarder_factory.h"
#include "chrome/browser/chromeos/policy/user_network_configuration_updater_factory.h"
#else
#include "chrome/browser/policy/cloud/user_cloud_policy_manager_factory.h"
#if !defined(OS_IOS)
#include "chrome/browser/policy/cloud/user_policy_signin_service_factory.h"
#endif
#endif
#endif

#if defined(ENABLE_MANAGED_USERS)
#include "chrome/browser/managed_mode/managed_user_service_factory.h"
#include "chrome/browser/managed_mode/managed_user_sync_service_factory.h"
#if defined(OS_CHROMEOS)
#include "chrome/browser/managed_mode/chromeos/managed_user_password_service_factory.h"
#include "chrome/browser/managed_mode/chromeos/manager_password_service_factory.h"
#endif
#endif

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/extensions/file_manager/file_browser_private_api_factory.h"
#include "chrome/browser/chromeos/extensions/input_method_api.h"
#include "chrome/browser/chromeos/extensions/media_player_api.h"
#include "chrome/browser/chromeos/extensions/screenlock_private_api.h"
#include "chrome/browser/extensions/api/input_ime/input_ime_api.h"
#include "chrome/browser/extensions/api/log_private/log_private_api.h"
#endif

#if defined(USE_AURA)
#include "chrome/browser/ui/gesture_prefs_observer_factory_aura.h"
#endif

#if defined(OS_ANDROID)
#include "chrome/browser/media/protected_media_identifier_permission_context_factory.h"
#else
#include "chrome/browser/media_galleries/media_galleries_preferences_factory.h"
#include "chrome/browser/notifications/sync_notifier/chrome_notifier_service_factory.h"
#include "chrome/browser/profile_resetter/automatic_profile_resetter_factory.h"
#endif

#if defined(ENABLE_APP_LIST)
#include "chrome/browser/ui/app_list/start_page_service_factory.h"
#endif

#if defined(ENABLE_SPELLCHECK)
#include "chrome/browser/extensions/api/spellcheck/spellcheck_api.h"
#include "chrome/browser/spellchecker/spellcheck_factory.h"
#endif

#if defined(ENABLE_MDNS)
#include "chrome/browser/local_discovery/privet_notifications_factory.h"
#endif

#if defined(ENABLE_PUSH_API)
#include "chrome/browser/push_registration/chrome_push_permission_context_factory.h"
#endif

namespace chrome {

void AddProfilesExtraParts(ChromeBrowserMainParts* main_parts) {
  main_parts->AddParts(new ChromeBrowserMainExtraPartsProfiles());
}

}  // namespace chrome

ChromeBrowserMainExtraPartsProfiles::ChromeBrowserMainExtraPartsProfiles() {
}

ChromeBrowserMainExtraPartsProfiles::~ChromeBrowserMainExtraPartsProfiles() {
}

// This method gets the instance of each ServiceFactory. We do this so that
// each ServiceFactory initializes itself and registers its dependencies with
// the global PreferenceDependencyManager. We need to have a complete
// dependency graph when we create a profile so we can dispatch the profile
// creation message to the services that want to create their services at
// profile creation time.
//
// TODO(erg): This needs to be something else. I don't think putting every
// FooServiceFactory here will scale or is desirable long term.
//
// static
void ChromeBrowserMainExtraPartsProfiles::
EnsureBrowserContextKeyedServiceFactoriesBuilt() {
#if defined(ENABLE_SIGNIN)
  AboutSigninInternalsFactory::GetInstance();
#endif
#if defined(ENABLE_APP_LIST)
  app_list::StartPageServiceFactory::GetInstance();
#endif
#if defined(ENABLE_AUTOFILL)
  autofill::PersonalDataManagerFactory::GetInstance();
#endif
#if !defined(OS_ANDROID)
  AutomaticProfileResetterFactory::GetInstance();
#endif
#if defined(ENABLE_BACKGROUND)
  BackgroundContentsServiceFactory::GetInstance();
#endif
  BookmarkModelFactory::GetInstance();
#if !defined(OS_ANDROID)
  BookmarkUndoServiceFactory::GetInstance();
#endif
#if defined(ENABLE_CAPTIVE_PORTAL_DETECTION)
  captive_portal::CaptivePortalServiceFactory::GetInstance();
#endif
  ChromeGeolocationPermissionContextFactory::GetInstance();
#if defined(ENABLE_PUSH_API)
  ChromePushPermissionContextFactory::GetInstance();
#endif
#if defined(OS_ANDROID)
  ProtectedMediaIdentifierPermissionContextFactory::GetInstance();
#endif
#if defined(ENABLE_FULL_PRINTING)
  CloudPrintProxyServiceFactory::GetInstance();
#endif
  CookieSettings::Factory::GetInstance();
#if defined(ENABLE_NOTIFICATIONS)
  DesktopNotificationServiceFactory::GetInstance();
#endif
  DownloadServiceFactory::GetInstance();
#if defined(ENABLE_EXTENSIONS)
  AppShortcutManagerFactory::GetInstance();
  apps::AppLoadServiceFactory::GetInstance();
  apps::AppRestoreServiceFactory::GetInstance();
  apps::AppWindowGeometryCache::Factory::GetInstance();
  EphemeralAppServiceFactory::GetInstance();
  extensions::ActivityLogFactory::GetInstance();
  extensions::ActivityLogAPI::GetFactoryInstance();
  extensions::AlarmManager::GetFactoryInstance();
  extensions::ApiResourceManager<extensions::ResumableTCPServerSocket>::
      GetFactoryInstance();
  extensions::ApiResourceManager<extensions::ResumableTCPSocket>::
      GetFactoryInstance();
  extensions::ApiResourceManager<extensions::ResumableUDPSocket>::
      GetFactoryInstance();
  extensions::ApiResourceManager<extensions::SerialConnection>::
      GetFactoryInstance();
  extensions::ApiResourceManager<extensions::Socket>::GetFactoryInstance();
  extensions::ApiResourceManager<extensions::UsbDeviceResource>::
      GetFactoryInstance();
  extensions::api::TCPServerSocketEventDispatcher::GetFactoryInstance();
  extensions::api::TCPSocketEventDispatcher::GetFactoryInstance();
  extensions::api::UDPSocketEventDispatcher::GetFactoryInstance();
  extensions::AudioAPI::GetFactoryInstance();
  extensions::BookmarksAPI::GetFactoryInstance();
  extensions::BluetoothAPIFactory::GetInstance();
  extensions::BrailleDisplayPrivateAPI::GetFactoryInstance();
  extensions::chromedirectsetting::ChromeDirectSettingAPI::GetFactoryInstance();
  extensions::CommandService::GetFactoryInstance();
  extensions::CookiesAPI::GetFactoryInstance();
  extensions::DeveloperPrivateAPIFactory::GetInstance();
  extensions::DialAPIFactory::GetInstance();
  extensions::ExtensionActionAPI::GetFactoryInstance();
  extensions::ExtensionPrefsFactory::GetInstance();
  extensions::ExtensionSystemFactory::GetInstance();
  extensions::ExtensionWebUIOverrideRegistrar::GetFactoryInstance();
  extensions::FeedbackPrivateAPI::GetFactoryInstance();
  extensions::FontSettingsAPI::GetFactoryInstance();
  extensions::HistoryAPI::GetFactoryInstance();
  extensions::HotwordPrivateEventService::GetFactoryInstance();
  extensions::IdentityAPI::GetFactoryInstance();
  extensions::IdleManagerFactory::GetInstance();
  extensions::InstallTrackerFactory::GetInstance();
#if defined(TOOLKIT_VIEWS)
  extensions::InputAPI::GetFactoryInstance();
#endif
#if defined(OS_CHROMEOS)
  extensions::InputImeAPI::GetFactoryInstance();
  extensions::InputMethodAPI::GetFactoryInstance();
  extensions::ScreenlockPrivateEventRouter::GetFactoryInstance();
#endif
  extensions::LocationManager::GetFactoryInstance();
#if defined(OS_CHROMEOS)
  extensions::LogPrivateAPI::GetFactoryInstance();
#endif
  extensions::ManagementAPI::GetFactoryInstance();
  extensions::MDnsAPI::GetFactoryInstance();
  extensions::MediaGalleriesPrivateAPI::GetFactoryInstance();
#if defined(OS_CHROMEOS)
  extensions::MediaPlayerAPI::GetFactoryInstance();
#endif
  extensions::MenuManagerFactory::GetInstance();
#if defined(OS_CHROMEOS) || defined(OS_WIN) || defined(OS_MACOSX)
  extensions::NetworkingPrivateEventRouterFactory::GetInstance();
#endif  // defined(OS_CHROMEOS) || defined(OS_WIN)
  extensions::OmniboxAPI::GetFactoryInstance();
#if defined(ENABLE_PLUGINS)
  extensions::PluginManager::GetFactoryInstance();
#endif  // defined(ENABLE_PLUGINS)
  extensions::PreferenceAPI::GetFactoryInstance();
  extensions::ProcessesAPI::GetFactoryInstance();
  extensions::PushMessagingAPI::GetFactoryInstance();
  extensions::RendererStartupHelperFactory::GetInstance();
  extensions::RuntimeAPIFactory::GetInstance();
  extensions::SessionsAPI::GetFactoryInstance();
  extensions::SettingsOverridesAPI::GetFactoryInstance();
  extensions::SignedInDevicesManager::GetFactoryInstance();
#if defined(ENABLE_SPELLCHECK)
  extensions::SpellcheckAPI::GetFactoryInstance();
#endif
  extensions::StreamsPrivateAPI::GetFactoryInstance();
  extensions::SystemInfoAPI::GetFactoryInstance();
  extensions::TabCaptureRegistry::GetFactoryInstance();
  extensions::TabsWindowsAPI::GetFactoryInstance();
  extensions::TtsAPI::GetFactoryInstance();
  extensions::WebNavigationAPI::GetFactoryInstance();
  extensions::WebRequestAPI::GetFactoryInstance();
  extensions::WebrtcAudioPrivateEventService::GetFactoryInstance();
  ExtensionToolbarModelFactory::GetInstance();
#endif  // defined(ENABLE_EXTENSIONS)
  FaviconServiceFactory::GetInstance();
#if defined(OS_CHROMEOS)
  file_manager::FileBrowserPrivateAPIFactory::GetInstance();
#endif
  FindBarStateFactory::GetInstance();
  GAIAInfoUpdateServiceFactory::GetInstance();
#if defined(USE_AURA)
  GesturePrefsObserverFactoryAura::GetInstance();
#endif
  GlobalErrorServiceFactory::GetInstance();
  GoogleURLTrackerFactory::GetInstance();
  HistoryServiceFactory::GetInstance();
  HotwordServiceFactory::GetInstance();
#if defined(ENABLE_SYNC)
  invalidation::InvalidationServiceFactory::GetInstance();
#endif
  InstantServiceFactory::GetInstance();
#if defined(ENABLE_MDNS)
  local_discovery::PrivetNotificationServiceFactory::GetInstance();
#endif
#if defined(ENABLE_MANAGED_USERS)
  ManagedUserServiceFactory::GetInstance();
  ManagedUserSyncServiceFactory::GetInstance();
#if defined(OS_CHROMEOS)
  ManagedUserPasswordServiceFactory::GetInstance();
  ManagerPasswordServiceFactory::GetInstance();
#endif
#endif
#if !defined(OS_ANDROID)
  MediaGalleriesPreferencesFactory::GetInstance();
  notifier::ChromeNotifierServiceFactory::GetInstance();
#endif
#if defined(ENABLE_WEBUI)        
  NTPResourceCacheFactory::GetInstance();
#endif
  PasswordStoreFactory::GetInstance();
#if !defined(OS_ANDROID)
  PinnedTabServiceFactory::GetInstance();
#endif
#if defined(ENABLE_PLUGINS)
  PluginPrefsFactory::GetInstance();
#endif
  policy::ProfilePolicyConnectorFactory::GetInstance();
#if defined(ENABLE_CONFIGURATION_POLICY)
#if defined(OS_CHROMEOS)
  policy::PolicyCertServiceFactory::GetInstance();
  policy::RecommendationRestorerFactory::GetInstance();
  policy::UserCloudPolicyManagerFactoryChromeOS::GetInstance();
  policy::UserCloudPolicyTokenForwarderFactory::GetInstance();
  policy::UserNetworkConfigurationUpdaterFactory::GetInstance();
#else
#if defined(ENABLE_SYNC)
  policy::UserCloudPolicyManagerFactory::GetInstance();
#endif //ENABLE_SYNC
#if !defined(OS_IOS) && defined(ENABLE_SIGNIN)
  policy::UserPolicySigninServiceFactory::GetInstance();
#endif
#endif
  policy::PolicyHeaderServiceFactory::GetInstance();
  policy::SchemaRegistryServiceFactory::GetInstance();
#if defined(ENABLE_SYNC)
  policy::UserCloudPolicyInvalidatorFactory::GetInstance();
#endif //ENABLE_SYNC
#endif
  predictors::AutocompleteActionPredictorFactory::GetInstance();
  predictors::PredictorDatabaseFactory::GetInstance();
#if defined(ENABLE_PRERENDER)
  prerender::PrerenderManagerFactory::GetInstance();
  prerender::PrerenderLinkManagerFactory::GetInstance();
#endif
#if defined(ENABLE_SYNC)
  ProfileSyncServiceFactory::GetInstance();
#endif
  ProtocolHandlerRegistryFactory::GetInstance();
#if defined(ENABLE_SESSION_SERVICE)
  SessionServiceFactory::GetInstance();
#endif
#if !defined (DISABLE_AUTOCOMPLETE)
  ShortcutsBackendFactory::GetInstance();
#endif
#if defined(ENABLE_SIGNIN)
  SigninManagerFactory::GetInstance();
#endif
#if defined(ENABLE_SPELLCHECK)
  SpellcheckServiceFactory::GetInstance();
#endif
#if defined(ENABLE_MOSTVISITED)
  ThumbnailServiceFactory::GetInstance();
#endif
  TabRestoreServiceFactory::GetInstance();
  TemplateURLFetcherFactory::GetInstance();
  TemplateURLServiceFactory::GetInstance();
#if defined(ENABLE_THEMES)
  ThemeServiceFactory::GetInstance();
#endif
#if defined(ENABLE_EXTENSIONS)
  TokenCacheServiceFactory::GetInstance();
#endif
  WebDataServiceFactory::GetInstance();
}

void ChromeBrowserMainExtraPartsProfiles::PreProfileInit() {
  EnsureBrowserContextKeyedServiceFactoriesBuilt();
}