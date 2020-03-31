/****************************************************************
 * file hcv_plugins.cc
 *
 * Description:
 *      Plugins machinery of https://github.com/bstarynk/helpcovid
 *
 * Author(s):
 *      © Copyright 2020
 *      Basile Starynkevitch <basile@starynkevitch.net>
 *      Abhishek Chakravarti <abhishek@taranjali.org>
 *
 *
 * License:
 *    This HELPCOVID program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "hcv_header.hh"

extern "C" const char hcv_plugins_gitid[] = HELPCOVID_GITID;
extern "C" const char hcv_plugins_date[] = __DATE__;

static std::recursive_mutex hcv_plugin_mtx;

typedef void hcvplugin_initweb_sig_t(httplib::Server*);

struct Hcv_plugin
{
  std::string hcvpl_name;
  void* hcvpl_handle; 		// result of dlopen
  std::string hcvpl_gitid;
  std::string hcvpl_license;
  hcvplugin_initweb_sig_t* hcvpl_initweb;
};

std::vector<Hcv_plugin> hcv_plugin_vect;

void hcv_load_plugin(const char*plugin)
{
  if (!plugin || !plugin[0])
    HCV_FATALOUT("missing plugin name");
  if (strlen(plugin) > HCV_PLUGIN_NAME_MAXLEN)
    HCV_FATALOUT("too long plugin name: " << plugin);
  for (const char*pc = plugin; *pc; pc++)
    if (!isalnum(*pc) && *pc != '_')
      HCV_FATALOUT("invalid plugin name " << plugin);
  std::lock_guard<std::recursive_mutex> guplug(hcv_plugin_mtx);
  HCV_DEBUGOUT("hcv_load_plugin " << plugin << " starting");
  std::string pluginstr(plugin);
  for (auto& pl : hcv_plugin_vect)
    if (pl.hcvpl_name == pluginstr)
      HCV_FATALOUT("hcv_load_plugin duplicate plugin " << plugin);
  char sobuf[HCV_PLUGIN_NAME_MAXLEN + 48];
  memset(sobuf, 0, sizeof(sobuf));
  snprintf(sobuf, sizeof(sobuf), HCV_PLUGIN_PREFIX "%s" HCV_PLUGIN_SUFFIX,
           plugin);
  HCV_ASSERT(sobuf[sizeof(sobuf)-1]==(char)0);
  HCV_ASSERT(strstr(sobuf, ".so") != nullptr);
  void* dlh = dlopen(sobuf, RTLD_NOW | RTLD_DEEPBIND);
  if (!dlh)
    HCV_FATALOUT("hcv_load_plugin " << plugin << " failed to dlopen " << sobuf
                 << " : " << dlerror());
  HCV_DEBUGOUT("hcv_load_plugin dlopened " << sobuf);
  const char* plgname
    = reinterpret_cast<const char*>(dlsym(dlh,
                                          "hcvplugin_name"));
  if (!plgname)
    HCV_FATALOUT("hcv_load_plugin " << plugin << " plugin " << sobuf
                 << " has no symbol hcvplugin_name: " << dlerror());
  if (strcmp(plgname, plugin))
    HCV_FATALOUT("hcv_load_plugin " << plugin << " plugin has unexpected hcvplugin_name " << plgname);
  const char* plglicense
    = reinterpret_cast<const char*>(dlsym(dlh,
                                          "hcvplugin_gpl_compatible_license"));
  if (!plglicense)
    HCV_FATALOUT("hcv_load_plugin " << plugin << " plugin " << sobuf
                 << " has no symbol hcvplugin_gpl_compatible_license: " << dlerror());
  const char* plgapi
    = reinterpret_cast<const char*>(dlsym(dlh, "hcvplugin_gitapi"));
  if (!plgapi)
    HCV_FATALOUT("hcv_load_plugin " << plugin << " plugin " << sobuf
                 << " has no symbol hcvplugin_gitapi: " << dlerror());
  const char* plgversion
    = reinterpret_cast<const char*>( dlsym(dlh, "hcvplugin_version"));
  if (!plgversion)
    HCV_FATALOUT("hcv_load_plugin " << plugin << " plugin " << sobuf
                 << " has no symbol hcvplugin_version: " << dlerror());
  void* plginit = dlsym(dlh, "hcvplugin_initialize_web");
  if (!plginit)
    HCV_FATALOUT("hcv_load_plugin " << plugin << " plugin " << sobuf
                 << " has no symbol hcvplugin_initialize_web: " << dlerror());
  HCV_SYSLOGOUT(LOG_NOTICE, "hcv_load_plugin " << plugin
                << " dlopened " << sobuf << " with license " << plglicense
                << " gitapi " << plgapi << " and version " << plgversion);
  if (strncmp(plgapi, hcv_gitid, 24))
    HCV_SYSLOGOUT(LOG_WARNING, "hcv_load_plugin " << plugin
                  << " dlopened " << sobuf
                  << " with gitapi mismatch - expected " << hcv_gitid
                  << " but got " << plgapi);
  hcv_plugin_vect.emplace_back
  (Hcv_plugin
  {
    .hcvpl_name = pluginstr,
    .hcvpl_handle= dlh,
    .hcvpl_gitid= std::string(plgapi),
    .hcvpl_license = std::string(plglicense),
    .hcvpl_initweb = reinterpret_cast<hcvplugin_initweb_sig_t*>(plginit)
  });
  ////
  HCV_DEBUGOUT("hcv_load_plugin done " << pluginstr << " rank#"
               << hcv_plugin_vect.size());
} // end hcv_load_plugin



void
hcv_initialize_plugins_for_web(httplib::Server*webserv)
{
  HCV_ASSERT(webserv != nullptr);
  std::lock_guard<std::recursive_mutex> guplug(hcv_plugin_mtx);
  auto nbplugins = hcv_plugin_vect.size();
  HCV_DEBUGOUT("hcv_initialize_plugins_for_web starting with " << nbplugins
               << " plugins");
  if (nbplugins == 0)
    return;
  for (auto& pl : hcv_plugin_vect)
    {
      HCV_DEBUGOUT("hcv_initialize_plugins_for_web initializing " << pl.hcvpl_name);
      (*pl.hcvpl_initweb)(webserv);
      HCV_SYSLOGOUT(LOG_INFO, "hcv_initialize_plugins_for_web initialized plugin "
                    << pl.hcvpl_name);
    };
  HCV_SYSLOGOUT(LOG_INFO, "hcv_initialize_plugins_for_web done with " << nbplugins
                << " plugins");
} // end hcv_initialize_plugins_for_web


/****************** end of file hcv_plugins.cc of github.com/bstarynk/helpcovid **/