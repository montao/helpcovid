/****************************************************************
 * file hcv_views.cc
 *
 * Description:
 *      Implementation of views processing HTTP requests.
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



extern "C" const char hcv_views_gitid[] = HELPCOVID_GITID;
extern "C" const char hcv_views_date[] = __DATE__;


std::string
hcv_login_view_get(const httplib::Request& req, httplib::Response& resp)
{
  if (req.method != "GET")
    HCV_FATALOUT("hcv_login_view_get() called with non GET request");

  Hcv_http_template_data data(req, resp, hcv_get_web_request_counter());
  std::string thtml = hcv_get_web_root() + "html/signin.html";

  return hcv_expand_template_file(thtml, &data);
} // end hcv_login_view_get


std::string
hcv_login_view_post(const httplib::Request& req,  httplib::Response& resp)
{
  if (req.method != "POST")
    HCV_FATALOUT("hcv_login_view_post() called with not POST request");

  auto email = req.get_param_value("email");
  auto passwd = req.get_param_value("password");

  HCV_DEBUGOUT("email=" << email);
  HCV_DEBUGOUT("passwd=" << passwd);

#warning user authentication and cookie setting needs to be implemented.
  Hcv_http_template_data data(req, resp, hcv_get_web_request_counter());
  std::string thtml = hcv_get_web_root() + "html/index.html";

  return hcv_expand_template_file(thtml, &data);
} // end hcv_login_view_post


std::string
hcv_home_view_get(const httplib::Request& req, httplib::Response& resp)
{
  if (req.method != "GET")
    HCV_FATALOUT("hcv_home_view_get() called with non GET request");

  // for now, redirect to the login view by default
  hcv_login_view_get(req, resp);
#warning implement conditional redirection based on session checks
}


//////////////////// end of file hcv_views.cc of github.com/bstarynk/helpcovid

