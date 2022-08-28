/*
 * SPDX-FileCopyrightText: 2022 traumweh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TRAUMWEH_IRCPP_HPP
#define TRAUMWEH_IRCPP_HPP
#include <functional>
#include <ixwebsocket/IXWebSocket.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace traumweh::ircpp {
struct IRCMessage {
  std::optional<std::string> raw;
  std::unordered_map<std::string, std::variant<bool, std::string>> tags;
  std::optional<std::string> prefix;
  std::string command;
  struct {
    std::optional<std::vector<std::string>> middle;
    std::optional<std::string> trailing;
  } params;
};

class Irc;
using conFunc = std::function<auto()->void>;
using msgFunc = std::function<auto(IRCMessage &message)->void>;
using errFunc = std::function<auto(std::string err)->void>;

class Irc {
public:
  /**
   * Construct an Irc connection that connects and reconnects automatically
   * (using exponential backoff) right away and parses incoming messages.
   *
   * @param host Hostname to connect to (ws or wss url)
   * @anchor<onCon>
   * @param onConnection Callback on successful connection
   * @param onMessage Callback on incoming message
   * @param onError Callback on error
   *
   */
  Irc(std::string host, conFunc onConection, msgFunc onMessage,
      errFunc onError);

  /**
   * Parse a single raw irc-message defined by
   * [RFC 1459](https://www.rfc-editor.org/rfc/rfc1459#section-2.3.1) and
   * [RFC 2812](https://www.rfc-editor.org/rfc/rfc2812#section-2.3.1).
   * Support for [message-tags](https://ircv3.net/specs/extensions/message-tags)
   *
   * @param msg single irc-message (MUST BE stripped of CRLF)
   * @return An optional parsed IRCMessage-object
   */
  static auto parseMessage(const std::string &msg) -> std::optional<IRCMessage>;

  /**
   * Helper method to authenticate to the irc-server and optionally request
   * server-capabilities.
   *
   * Recommended to be called in the @ref<onCon>-callback.
   *
   * @param pass An authentication string, e.g. "oauth:abcdefg0123456789"
   * @param nick User to authenticate as
   * @param caps Optional capabilities (comma-separated), see
   *             [IRCv3](https://ircv3.net/specs/extensions/capability-negotiation.html)
   */
  auto authenticate(std::string pass, std::string nick,
                    std::optional<std::string> caps = {}) -> void;

  /**
   * Helper method to join a channel on the irc-server.
   *
   * @param channel The name of the channel (case sensitive!)
   */
  auto join(std::string channel) -> void;

  /**
   * @anchor<write>
   * Send a message to the irc-server.
   *
   * @param msg A valid irc message
   */
  auto write(const std::string &msg) -> void;

  /**
   * Overload helper method for @ref<write>
   * Send a message to the irc-server.
   *
   * Data in the IRCMessage-object need to be properly escaped!
   *
   * @param msg An IRCMessage-object
   */
  auto write(const IRCMessage &msg) -> void;

  /**
   * Helper-method to split a string into substrings around a string delimiter.
   *
   * The substrings won't contain the delimiter anymore!
   *
   * @param str string to split
   * @param sep seperator to split around
   */
  static auto splitAtSep(std::string str, std::string sep)
      -> std::vector<std::string>;

private:
  std::string host;
  conFunc onCon;
  msgFunc onMsg;
  errFunc onErr;
  ix::WebSocket webSocket;

  /**
   * Split message-batch by CRLF, parse every single message and answer a ping
   * with a pong.
   *
   * If the message is a valid irc message, the message-callback will be called
   * with it; even for PING-messages which are already handled.
   *
   * @param batch Batch-reply from the irc-server containing multiple messages
   *              delimited by CRLF.
   */
  auto splitBatch(const std::string batch) -> void;
};
} // namespace traumweh::ircpp

#endif
