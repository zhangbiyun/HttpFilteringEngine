/*
* Copyright (c) 2016 Jesse Nicholson.
*
* This file is part of Http Filtering Engine.
*
* Http Filtering Engine is free software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or (at
* your option) any later version.
*
* In addition, as a special exception, the copyright holders give
* permission to link the code of portions of this program with the OpenSSL
* library.
*
* You must obey the GNU General Public License in all respects for all of
* the code used other than OpenSSL. If you modify file(s) with this
* exception, you may extend this exception to your version of the file(s),
* but you are not obligated to do so. If you do not wish to do so, delete
* this exception statement from your version. If you delete this exception
* statement from all source files in the program, then also delete it
* here.
*
* Http Filtering Engine is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with Http Filtering Engine. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>

#include "util/cb/EventReporter.hpp"
#include "mitm/secure/TlsCapableHttpAcceptor.hpp"

namespace te
{
	namespace httpengine
	{
		namespace mitm
		{
			namespace diversion
			{
				class DiversionControl;
			} /* namespace diversion */
		} /* namespace mitm */
	} /* namespace httpengine */
} /* namespace te */

namespace te
{
	namespace httpengine
	{

		/// <summary>
		/// The HttpFilteringEngineControl class is the managing class that employs all other
		/// classes in this Engine to provide the combined functionality of intercepting and
		/// diverting HTTP/S traffic, a transparent proxy listening for and handling the traffic for
		/// those diverted clients, and the Http Filtering Engine for inspecting and filtering
		/// requests and response payloads based on user loaded rulesets.
		/// </summary>
		class HttpFilteringEngineControl : public util::cb::EventReporter
		{

		public:

			/// <summary>
			/// Constructs a new HttpFilteringEngineControl. Requires a valid firewall callback
			/// function pointer on Windows or the constructor will throw. Optionally, callbacks for
			/// information, warning and error events within the underlying Engine can be supplied as
			/// well.
			/// </summary>
			/// <param name="firewallCb">
			/// A function that is meant to determine if the supplied absolute binary path points to
			/// a binary that has been approved for internet access. Required.
			/// </param>
			/// <param name="caBundleAbsolutePath">
			/// A an absolute path to a CA bundle that will be used globally while acting as the
			/// upstream client on behalf of all downstream client connections for the purpose of
			/// verifying server certificates. Optional and default value is "none", which means none
			/// will be configured internally. It is recommended to supply a path to the cURL/Mozilla
			/// ca-bundle. Internally, openSSL is set to use default verify path(s), but this is
			/// configuration and platform specific. If this fails and no CA bundle is supplied,
			/// TLS/SSL will basically be non functional.
			/// </param>
			/// <param name="blockedHtmlPage">
			/// An optional, but recommended, pointer to a HTML string to display whenever a HTML
			/// payload is blocked.
			/// </param>
			/// <param name="httpListenerPort">
			/// The desired port for the proxy to listen for incoming plain TCP HTTP clients on.
			/// Default is zero, as it is recommended to allow the OS to select an available port
			/// from the ephimeral port range.
			/// </param>
			/// <param name="httpsListenerPort">
			/// The desired port for the proxy to listen for incoming secure HTTP clients on. Default
			/// is zero, as it is recommended to allow the OS to select an available port from the
			/// ephimeral port range.
			/// </param>
			/// <param name="proxyNumThreads">
			/// The number of thread to be run against the io_service that drives the proxy and all
			/// associated functionality, barring the platform dependent Diverter. Default value is
			/// the number of logical cores on the device. Be advised that these threads are the same
			/// threads that execute the filtering functionality.
			/// </param>
			/// <param name="onClassify">
			/// A function that can accept content in the form of an array of bytes, along with a
			/// string identifying the content type, and then return a category that the supplied
			/// data belongs to. Default is nullptr. This callback cannot be supplied
			/// post-construction.
			/// </param>
			/// <param name="onInfo">
			/// A function that can accept string informational data generated by the underlying
			/// Engine. Default is nullptr. This callback cannot be supplied post-construction.
			/// </param>
			/// <param name="onWarn">
			/// A function that can accept string warning data generated by the underlying Engine.
			/// Default is nullptr. This callback cannot be supplied post-construction.
			/// </param>
			/// <param name="onError">
			/// A function that can accept string error data generated by the underlying Engine.
			/// Default is nullptr. This callback cannot be supplied post-construction.
			/// </param>
			/// <param name="onRequestBlocked">
			/// A function that can accept information about blocked requests generated by the
			/// underlying Engine. Default is nullptr. This callback cannot be supplied
			/// post-construction.
			/// </param>
			/// <param name="onElementsBlocked">
			/// A function that can accept information about HTML elements removed by CSS selects,
			/// generated by the underlying Engine. Default is nullptr. This callback cannot be
			/// supplied post-construction.
			/// </param>
			HttpFilteringEngineControl(
				util::cb::FirewallCheckFunction firewallCb,				
				std::string caBundleAbsolutePath = std::string(u8"none"),
				std::string blockedHtmlPage = std::string(),
				uint16_t httpListenerPort = 0,
				uint16_t httpsListenerPort = 0,
				uint32_t proxyNumThreads = std::thread::hardware_concurrency(),
				util::cb::ContentClassificationFunction onClassify = nullptr,
				util::cb::MessageFunction onInfo = nullptr,
				util::cb::MessageFunction onWarn = nullptr,
				util::cb::MessageFunction onError = nullptr,
				util::cb::RequestBlockFunction onRequestBlocked = nullptr,
				util::cb::ElementBlockFunction onElementsBlocked = nullptr
				);

			/// <summary>
			/// Default destructor.
			/// </summary>
			~HttpFilteringEngineControl();

			/// <summary>
			/// If the underlying Engine is not running at the time that this method is invoked, the
			/// Engine will begin diverting traffic to itself and listening for incoming diverted
			/// HTTP and HTTPS connections to filter. If the underlying Engine is already running,
			/// the call will have no effect.
			/// 
			/// Expect this function to potentially throw std::runtime_error and std::exception.
			/// </summary>
			void Start();

			/// <summary>
			/// If the underlying Engine is running at the time that this method is invoked, the
			/// Engine will cease diverting traffic to itself and cease listening for incoming
			/// diverted HTTP and HTTPS connections. If the underlying Engine is not running, the
			/// call will have no effect.
			/// </summary>
			void Stop();

			/// <summary>
			/// Checks whether the underlying Engine and its associated mechanisms are presently
			/// diverting traffic to itself and listening for incoming diverted HTTP and HTTPS
			/// connections to filter.
			/// </summary>
			/// <returns>
			/// True if the underlying Engine is actively diverting and receiving HTTP and HTTPS
			/// connections for filtering at the time of the call, false otherwise.
			/// </returns>
			bool IsRunning() const;

			/// <summary>
			/// Gets the port on which the plain TCP HTTP acceptor is listening.
			/// </summary>
			/// <returns>
			/// If the Engine is running, the port on which the plain TCP HTTP acceptor is
			/// listening. Zero otherwise.
			/// </returns>
			const uint32_t GetHttpListenerPort() const;

			/// <summary>
			/// Gets the port on which the secure HTTP acceptor is listening.
			/// </summary>
			/// <returns>
			/// If the Engine is running, the port on which the plain TCP HTTP acceptor is
			/// listening. Zero otherwise.
			/// </returns>
			const uint32_t GetHttpsListenerPort() const;

			/// <summary>
			/// Sets the state of a program-wide option. These options are implemented as an array of
			/// atomics. Effects of modifying options should be seen immediately and requires no
			/// locking or restarting.
			/// </summary>
			/// <param name="option">
			/// The option represented by the 32 bit unsigned integer. These options have a fixed
			/// maximum index that is much shorter than the numeric limits. A header should be
			/// provided per language that gives appropriate indices, named to represent the meaning
			/// of the option. If the supplied option value is beyond the limits, the call will
			/// succeed but have no effect. XXX TODO - Possibly change return to bool to reflect
			/// true success/fail?
			/// </param>
			/// <param name="enabled">
			/// The value to set the supplied option to.
			/// </param>
			void SetOptionEnabled(const uint32_t option, const bool enabled);

			/// <summary>
			/// Checks the current value of the supplied option.
			/// </summary>
			/// <param name="option">
			/// The option represented by the 32 bit unsigned integer. These options have a fixed
			/// maximum index that is much shorter than the numeric limits. A header should be
			/// provided per language that gives appropriate indices, named to represent the meaning
			/// of the option.
			/// </param>
			/// <returns>
			/// The current value of the supplied option. If the supplied option is outside the
			/// limits of the total available options, the return value will always be false.
			/// </returns>
			const bool GetOptionEnabled(const uint32_t option) const;

			/// <summary>
			/// Sets the state of a program-wide filtering category. These categories are
			/// implemented as an array of atomics. Effects of modifying options should be seen
			/// immediately and requires no locking or restarting.
			/// 
			/// Unlike the filtering options, the Engine is virtually agnostic to the meaning of
			/// category values, except the value zero. Zero is reserved to imply that a request or
			/// some other filterable element should not be filtered at all. Other than that, the
			/// implied meaning of these values can be entirely made up by the user. The Engine
			/// cares not what category 1 is. It simply checks if the user has loaded any rules that
			/// the user assigned to category 1 and, if something is matched by a rule in category
			/// 1, whether or not that rule category is enabled. If it is enabled, the filtering
			/// will take place.
			/// 
			/// The user can make use of any value ranging from 1 to std::numeric_limits::max() for
			/// the uint8_t type.
			/// </summary>
			/// <param name="category">
			/// The category. Must be non-zero. Zero values are ignored.
			/// </param>
			/// <param name="enabled">
			/// The value to set the supplied category to.
			/// </param>
			void SetCategoryEnabled(const uint8_t category, const bool enabled);

			/// <summary>
			/// Gets the current value set for the supplied category.
			/// </summary>
			/// <param name="category">
			/// The category to check. Note that supplying zero will always result in a return value
			/// of "false".
			/// </param>
			/// <returns>
			/// The current value of the supplied category.
			/// </returns>
			const bool GetCategoryEnabled(const uint8_t category) const;

			/// <summary>
			/// Attempts to load a list populated with Adblock Plus formatted filters and CSS
			/// selector rules. The underlying component that performs actually loading, parsing and
			/// storing these rules uses a Reader/Writer mutex system to ensure that users can flush
			/// and reload rules at will without requiring a restart, and without worrying about
			/// synchronization.
			/// </summary>
			/// <param name="filePath">
			/// The absolute path to the list to be loaded.
			/// </param>
			/// <param name="listCategory">
			/// The category to assign to the rules loaded from the list. This allows certain rules
			/// to be enabled and disabled at runtime by the user.
			/// </param>
			/// <param name="flushExistingInCategory">
			/// Whether or not to release all current rules which are of the same category before
			/// loading the rules from the supplied list. Default value is true. This is to give the
			/// user the ability to load multiple lists which apply to the same category, but
			/// contain unique rules, consecutively. Take care when suppling the value "false", as
			/// no measure is taken to detect/prevent duplicate rule entries.
			/// </param>
			/// <param name="rulesLoaded">
			/// The total number of rules successfully loaded and parsed from the source.
			/// </param>
			/// <param name="rulesFailed">
			/// The total number of rules that failed to load and or be parsed from the source.
			/// </param>
			void LoadFilteringListFromFile(
				const std::string& filePath, 
				const uint8_t listCategory, 
				const bool flushExistingInCategory = true,
				uint32_t* rulesLoaded = nullptr,
				uint32_t* rulesFailed = nullptr
				);

			/// <summary>
			/// Attempts to parse the supplied string, which should be populated with Adblock Plus
			/// formatted filters and CSS selector rules. The underlying component that performs
			/// actually loading, parsing and storing these rules uses a Reader/Writer mutex system
			/// to ensure that users can flush and reload rules at will without requiring a restart,
			/// and without worrying about synchronization.
			/// </summary>
			/// <param name="listString">
			/// The list contents.
			/// </param>
			/// <param name="listCategory">
			/// The category to assign to the rules parsed from the list. This allows certain rules
			/// to be enabled and disabled at runtime by the user.
			/// </param>
			/// <param name="flushExistingInCategory">
			/// Whether or not to release all current rules which are of the same category before
			/// loading the rules from the supplied list. Default value is true. This is to give the
			/// user the ability to load multiple lists which apply to the same category, but
			/// contain unique rules, consecutively. Take care when suppling the value "false", as
			/// no measure is taken to detect/prevent duplicate rule entries.
			/// </param>
			/// <param name="rulesLoaded">
			/// The total number of rules successfully loaded and parsed from the source.
			/// </param>
			/// <param name="rulesFailed">
			/// The total number of rules that failed to load and or be parsed from the source.
			/// </param>
			void LoadFilteringListFromString(
				const std::string& listString, 
				const uint8_t listCategory, 
				const bool flushExistingInCategory = true, 
				uint32_t* rulesLoaded = nullptr,
				uint32_t* rulesFailed = nullptr
				);

			/// <summary>
			/// Loads text keywords from a file. Each unique keyword must be on a newline
			/// within the file. Note that text triggers should be used sparingly. You should
			/// only really use entries highly specific to content that you really don't want
			/// to get through, such as pornography. Any payload that is text based will be
			/// subjected to filtering via these triggers, so beware. You want want
			/// non-specific/common text as a trigger.
			/// </summary>
			/// <param name="triggers">
			/// The string holding the newline-delimited list of trigger words.
			/// </param>
			/// <param name="category">
			/// The category that extracted triggers belong to.
			/// </param>
			/// <param name="flushExisting">
			/// Whether or not to flush existing triggers before loading the new ones.
			/// </param>
			/// <returns>
			/// The total number of triggers loaded from the provided source.
			/// </returns>
			uint32_t LoadTextTriggersFromFile(const std::string& triggersFilePath, const uint8_t category, const bool flushExisting);

			/// <summary>
			/// Loads text keywords from a string. Each unique keyword must be on a newline.
			/// Note that text triggers should be used sparingly. You should only really use
			/// entries highly specific to content that you really don't want to get through,
			/// such as pornography. Any payload that is text based will be subjected to
			/// filtering via these triggers, so beware. You want want non-specific/common
			/// text as a trigger.
			/// </summary>
			/// <param name="triggers">
			/// The string holding the newline-delimited list of trigger words.
			/// </param>
			/// <param name="category">
			/// The category that extracted triggers belong to.
			/// </param>
			/// <param name="flushExisting">
			/// Whether or not to flush existing triggers before loading the new ones.
			/// </param>
			/// <returns>
			/// The total number of triggers loaded from the provided source.
			/// </returns>
			uint32_t LoadTextTriggersFromString(const std::string& triggers, const uint8_t category, const bool flushExisting);

			/// <summary>
			/// Gets a copy of the root certificate, if any, in PEM format.
			/// </summary>
			/// <returns>
			/// On success, a vector populated with the bytes for the current root CA in PEM
			/// format. In the event that an error occurred or there is no current root CA,
			/// an empty vector.
			/// </returns>
			std::vector<char> GetRootCertificatePEM() const;

			/// <summary>
			/// Unloads and and all rules created for the given category.
			/// </summary>
			/// <param name="category">
			/// The category for which to unload any and all rules.
			/// </param>
			void UnloadRulesForCategory(const uint8_t category);

			/// <summary>
			/// Unloads and and all text triggers created for the given category.
			/// </summary>
			/// <param name="category">
			/// The category for which to unload any and all text triggers.
			/// </param>
			void UnloadTextTriggersForCategory(const uint8_t category);

		private:

			/// <summary>
			/// If defined, called whenever a packet flow is being considered for diversion to the
			/// proxy, but the binary responsible for sending or receiving the flow has not yet been
			/// identified as a binary permitted to have internet access by the system firewall. If
			/// defined and the return from this callback is true, the binary has permission to
			/// access the internet, and diversion will take place. If false, no diversion will take place.
			/// 
			/// The purpose of this check is to avoid allowing an arbitrary program that would
			/// otherwise be blocked from accessing the internet, to access the internet. Since
			/// intercepted packets are never sent outbound, but rather this software acts as an
			/// agent to fulfill the request(s) itself, an application firewall would not be able to
			/// stop us from bypassing it on behalf of other software, once it has permitted this
			/// software to have internet access.
			/// </summary>
			util::cb::FirewallCheckFunction m_firewallCheckCb = nullptr;			

			/// <summary>
			/// The absolute path provided for a CA bundle to configure the upstream client context
			/// to use in certificate verification. This is only held as a class member because the
			/// underlying mechanism that takes this argument is not initialized in-constructor.
			/// It's necessary to store it here.
			/// </summary>
			std::string m_caBundleAbsolutePath;

			/// <summary>
			/// The desired port on which the proxy will listen for plain TCP HTTP clients. This is
			/// only held as a class member because the underlying mechanism that takes this
			/// argument is not initialized in-constructor. It's necessary to store it here. This is
			/// not to be returned in the public getter, since the listner itself may have bound to
			/// a different port when this argument is zero.
			/// </summary>
			uint16_t m_httpListenerPort;

			/// <summary>
			/// The desired port on which the proxy will listen for secure HTTP clients. This is
			/// only held as a class member because the underlying mechanism that takes this
			/// argument is not initialized in-constructor. It's necessary to store it here. This is
			/// not to be returned in the public getter, since the listner itself may have bound to
			/// a different port when this argument is zero.
			/// </summary>
			uint16_t m_httpsListenerPort;

			/// <summary>
			/// The number of threads to be run against the main io_service.
			/// </summary>
			uint32_t m_proxyNumThreads;

			/// <summary>
			/// Container for the threads driving the io_service.
			/// </summary>
			std::vector<std::thread> m_proxyServiceThreads;

			/// <summary>
			/// Options users can configure at runtime to modify the Engine functionality.
			/// </summary>
			std::unique_ptr<filtering::options::ProgramWideOptions> m_programWideOptions = nullptr;

			/// <summary>
			/// The underlying filtering Engine responsible for blocking request and removing HTML
			/// elements with CSS selectors.
			/// </summary>
			std::unique_ptr<filtering::http::HttpFilteringEngine> m_httpFilteringEngine = nullptr;

			/// <summary>
			/// The io_service that will drive the proxy.
			/// </summary>
			std::unique_ptr<boost::asio::io_service> m_service = nullptr;

			/// <summary>
			/// The certificate store that will be used for secure clients.
			/// </summary>
			std::unique_ptr<mitm::secure::BaseInMemoryCertificateStore> m_store = nullptr;

			/// <summary>
			/// The diversion class that is responsible for diverting HTTP and HTTPS flows to the
			/// HTTP and HTTPS listeners for filtering.
			/// </summary>
			std::unique_ptr<mitm::diversion::DiversionControl> m_diversionControl = nullptr;			

			/// <summary>
			/// Our acceptor for plain TCP HTTP clients.
			/// </summary>
			std::unique_ptr<mitm::secure::TcpAcceptor> m_httpAcceptor = nullptr;

			/// <summary>
			/// Our acceptor for secure TLS HTTP clients.
			/// </summary>
			std::unique_ptr<mitm::secure::TlsAcceptor> m_httpsAcceptor = nullptr;		

			/// <summary>
			/// Used in ::Start() ::Stop() members.
			/// </summary>
			std::mutex m_ctlMutex;

			/// <summary>
			/// Used to indicate if all compontents were initialized and started correctly, and are
			/// currently handling the process of diverting HTTP and HTTPS clients to the proxy to
			/// be served.
			/// </summary>
			std::atomic_bool m_isRunning;

		};

	} /* namespace httpengine */
} /* namespace te */
