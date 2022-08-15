# IRC Parser and Client Library in C++17

This library was originally intended to become a Twitch bot, but was separated
as its own library, because I couldn't find a library that did the job and
thought maybe someone else might benefit from this.

Parser follows [RFC 1459](https://www.rfc-editor.org/rfc/rfc1459#section-2.3.1) and 
[RFC 2812](https://www.rfc-editor.org/rfc/rfc2812#section-2.3.1), but also supports
[IRCv3 message-tags](https://ircv3.net/specs/extensions/message-tags) and 
[IRCv3 capabilities](https://ircv3.net/specs/extensions/capability-negotiation.html).

## Requirements

- C++17
- IXWebSocket
- Meson
- CMake

## Getting Started
The constructor `traumweh::ircpp::Irc` takes a host and three callbacks (static or lambda functions):

```cpp
std::string host; // "ws://..." or "wss://..."
std::function<auto()->void> onConnection;
std::function<auto(IRCMessage &message)->void> onMessage;
std::function<auto(std::string err)->void> onError;

traumweh::ircpp::Irc irc(host, onConnection, onMessage, onError);
```

On successful connection you can easily handle authentication, login and 
channel-joining. If you are using Oauth-authentication I recommend replacing 
`auth` by a function call or variable that is always containing a valid token, 
so that on token-refreshing the bot can automatically reconnect.

```cpp
onConnection = [irc]() -> void {
  std::cerr << "Connection established\n";
  irc.authenticate(auth, username);
  // optional: Capabilities of IRCv3, e.g. "twitch.tv/tags"
  irc.authenticate(auth, username, capabilities);
  irc.join(channel);
};
```

Once connected the client will receive messages which automatically get parsed. 
For every message `onMessage` will be called with an `IRCMessage` instance that 
follows [RFC 1459](https://www.rfc-editor.org/rfc/rfc1459#section-2.3.1), 
[RFC 2812](https://www.rfc-editor.org/rfc/rfc2812#section-2.3.1) as well as 
[IRCv3 message-tags](https://ircv3.net/specs/extensions/message-tags).

Note that `PING :<token>` messages will automatically be answered with a 
`PONG :<token>` as well as being used to call `onMessage`!

Here is an example for a simple `!ping` command:

```cpp
onMessage = [irc](traumweh::ircpp::IRCMessage &msg) -> void {
  if (msg.command == "NOTICE" && 
      msg.params.trailing == "Login authentication failed") {
    std::cerr << "Login authentication failed!\n"; // maybe Token needs to be refreshed
  } else if (msg.command == "PRIVMSG") {
    if (msg.params.trailing.rfind("!ping", 0) == 0) { // example !ping chatbot-command
      // you can send messages either by sending an irc message
      irc.write("@reply-parent-msg-id=" + msg.tags["user-id"] + 
                " PRIVMSG #" + channel + ":pong!");
      // or by creating an `IRCMessage`-object
      irc.write({{}, 
                 {{"reply-parent-msg-id", std::get<std::string>(msg.tags["user-id"])}},
                 {},
                 "PRIVMSG",
                 {{{"#" + channel}},
                 "pong!"}};
    }
  }
};
```

The last function `onError` gets called when the websocket encounteres an error 
and might give you insight of what went wrong, but might just be a simple 
disconnect by the irc-server which should cause the client to automatically 
reconnect using exponential backoff.

```cpp
onError = [irc](std::string err) -> void {
  std::cerr << "Connection error: " + err + "\n";
};
```

## Installation
If you are using meson, simply clone this repository to your `subprojects` 
directory and add the following dependency to your `meson.build`:
```
ircpp_dep = dependency('libircpp', fallback: 'ircpp')
```
