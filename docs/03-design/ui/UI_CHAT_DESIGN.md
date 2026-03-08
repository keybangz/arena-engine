# Arena Engine Chat UI System - Technical Design Document

**Version:** 1.0  
**Date:** 2026-03-08  
**Status:** Design Phase  
**Target Implementation:** 8-10 weeks

---

## Table of Contents

1. [Chat System Overview](#1-chat-system-overview)
2. [Channel Types](#2-channel-types)
3. [Chat Window Design](#3-chat-window-design)
4. [Input System](#4-input-system)
5. [Commands](#5-commands)
6. [Message Rendering](#6-message-rendering)
7. [Notification System](#7-notification-system)
8. [Moderation Features](#8-moderation-features)
9. [Chat History](#9-chat-history)
10. [Network Protocol](#10-network-protocol)
11. [UI Implementation](#11-ui-implementation)
12. [Code Structures](#12-code-structures)
13. [Integration Points](#13-integration-points)
14. [Accessibility](#14-accessibility)
15. [Performance](#15-performance)
16. [Implementation Roadmap](#16-implementation-roadmap)

---

## 1. Chat System Overview

### 1.1 Chat Contexts

The Arena Engine chat system operates across three distinct contexts:

#### **Launcher Context**
- Pre-game social hub
- Friend list integration
- Party formation
- Persistent chat with friends
- Multiple conversation tabs
- Pop-out window support

#### **In-Game Context**
- Real-time team communication
- All-chat for cross-team interaction
- Integrated into HUD
- Minimal visual footprint
- Quick access via hotkeys
- Fade-out when inactive

#### **Post-Game Context**
- Match summary chat
- GG/FF discussions
- Friend requests
- Report functionality
- Replay sharing

### 1.2 Channel Types and Purposes

| Channel | Visibility | Scope | Default | Hotkey |
|---------|-----------|-------|---------|--------|
| **Team Chat** | Team only (5 players) | In-game | Enabled | Enter |
| **All Chat** | All players (10 players) | In-game | Disabled | Shift+Enter |
| **Party Chat** | Party members | Lobby | Enabled | N/A |
| **Whisper** | 1-on-1 private | Any | On-demand | /w |
| **Club Chat** | Club members | Persistent | Optional | N/A |

### 1.3 Message Types

```c
typedef enum ChatMessageType {
    CHAT_MSG_TEXT,              // Regular player message
    CHAT_MSG_SYSTEM,            // System notification (join/leave)
    CHAT_MSG_GAME_EVENT,        // In-game event (kill, objective)
    CHAT_MSG_NOTIFICATION,      // UI notification (friend online)
    CHAT_MSG_EMOTE,             // Emote/action message
} ChatMessageType;
```

### 1.4 Moderation Features

- **Mute System:** Mute individual players or all chat
- **Report System:** Flag inappropriate behavior
- **Profanity Filter:** Optional word filtering
- **Rate Limiting:** Anti-spam protection
- **Auto-Moderation:** Flagged word detection

---

## 2. Channel Types

### 2.1 Team Chat (In-Game)

**Purpose:** Primary in-game communication channel for coordinated team play

**Specifications:**
- **Visibility:** Only visible to 5 teammates
- **Default State:** Enabled
- **Hotkey:** Enter
- **Color Coding:** Green/Blue (team color)
- **Character Limit:** 256 characters
- **Max Messages Displayed:** 50 (configurable)
- **Fade Timeout:** 10 seconds of inactivity

**Features:**
- Automatic focus on hotkey press
- Tab completion for teammate names
- Mention highlighting (@PlayerName)
- Timestamp display (optional)
- Scrollback history (last 100 messages)

**Example Display:**
```
[12:34] Player1: Let's group mid
[12:35] Player2: On my way
[12:36] * Player3 has connected
[12:37] Player1: @Player2 wait for me
```

### 2.2 All Chat (In-Game)

**Purpose:** Cross-team communication for sportsmanship and banter

**Specifications:**
- **Visibility:** All 10 players
- **Default State:** Disabled (can be toggled)
- **Hotkey:** Shift+Enter
- **Color Coding:** White/Gray
- **Character Limit:** 256 characters
- **Max Messages Displayed:** 30 (configurable)
- **Fade Timeout:** 15 seconds of inactivity

**Features:**
- Optional enable/disable toggle
- Separate message history
- Can be muted globally
- Profanity filter applies
- Rate limiting per player

**Example Display:**
```
[12:40] Enemy1: gg wp
[12:41] Player1: thanks, you too
[12:42] Enemy2: that was close
```

### 2.3 Party Chat (Lobby)

**Purpose:** Pre-game coordination and party communication

**Specifications:**
- **Visibility:** Party members only
- **Scope:** Persists through champion select
- **Character Limit:** 256 characters
- **Max Messages Displayed:** 100
- **Persistence:** Stored until party dissolves

**Features:**
- Integrated with party system
- Shows player ready status
- Champion selection notifications
- Rune/item discussion
- Persists across queue waits

**Example Display:**
```
[Party] Player1: Let's go mid priority
[Party] Player2: I'll support
[Party] * Player3 selected Jinx
[Party] Player1: Nice pick!
```

### 2.4 Whisper/Direct Message

**Purpose:** Private 1-on-1 communication

**Specifications:**
- **Visibility:** Sender and recipient only
- **Scope:** Persistent across sessions
- **Character Limit:** 500 characters
- **Max Messages Displayed:** 200 per conversation
- **Notification:** Toast popup + sound

**Commands:**
```
/w PlayerName message     - Send whisper
/msg PlayerName message   - Alias for /w
/r message                - Reply to last whisper
```

**Features:**
- Tab completion for friend names
- Conversation history
- Unread message indicator
- Toast notification for new messages
- Typing indicator (optional)
- Read receipts (optional)

**Example Display:**
```
[Whisper] You: Hey, want to queue?
[Whisper] Friend: Sure! Give me 5 mins
[Whisper] You: Cool, I'll wait
```

### 2.5 Club/Guild Chat (Optional)

**Purpose:** Persistent group communication for club members

**Specifications:**
- **Visibility:** Club members only
- **Scope:** Persistent across sessions
- **Character Limit:** 256 characters
- **Max Messages Displayed:** 500 (paginated)
- **Persistence:** Server-side storage

**Features:**
- Multiple clubs per player
- Club-specific channels (optional)
- Member list with status
- Club announcements
- Moderation by club officers

### 2.6 System Messages

**Purpose:** Inform players of game events and state changes

**Specifications:**
- **Color Coding:** Yellow/Orange
- **Format:** `* [Event] Description`
- **Examples:**
  - `* Player1 has connected`
  - `* Player2 has disconnected`
  - `* Player1 killed Player2`
  - `* Team has destroyed enemy tower`
  - `* Server maintenance in 5 minutes`

**System Message Types:**
```c
typedef enum SystemMessageType {
    SYS_PLAYER_JOINED,
    SYS_PLAYER_LEFT,
    SYS_PLAYER_DISCONNECTED,
    SYS_PLAYER_RECONNECTED,
    SYS_KILL_FEED,
    SYS_OBJECTIVE_DESTROYED,
    SYS_GAME_EVENT,
    SYS_SERVER_ANNOUNCEMENT,
} SystemMessageType;
```

---

## 3. Chat Window Design

### 3.1 In-Game Chat Panel

**Position:** Bottom-left corner of screen  
**Default Size:** 400px × 200px  
**Resizable:** Yes (with handle)  
**Dockable:** No (fixed position)  
**Transparency:** 70% (configurable)

**Layout:**
```
┌─────────────────────────────────────┐
│ [Team ▼] [Minimize] [Settings] [X] │  ← Header (30px)
├─────────────────────────────────────┤
│ [12:34] Player1: Hello team        │
│ [12:35] Player2: gl hf             │
│ [12:36] * Player3 has connected    │
│                                     │
│                                     │
│                                     │
├─────────────────────────────────────┤  ← Scrollbar (right edge)
│ [Type message...]         [⏎]      │  ← Input (30px)
└─────────────────────────────────────┘
```

**Components:**

#### **Header Bar (30px)**
- Channel selector dropdown (Team/All/Whisper)
- Minimize button (collapse to title bar)
- Settings button (opens preferences)
- Close button (hide chat)

#### **Message Display Area**
- Scrollable message list
- Semi-transparent background (rgba: 0, 0, 0, 0.7)
- Monospace font for readability
- Line wrapping at panel width
- Vertical scrollbar (auto-hide when inactive)

#### **Input Field (30px)**
- Text input with placeholder "Type message..."
- Character counter (optional)
- Send button or Enter to send
- Escape to cancel/unfocus

### 3.2 Launcher Chat Panel

**Position:** Dockable side panel  
**Default Size:** 300px × 600px  
**Resizable:** Yes  
**Dockable:** Yes (left/right/floating)  
**Transparency:** 100% (opaque)

**Layout:**
```
┌──────────────────────────┐
│ Friends & Chat           │
├──────────────────────────┤
│ [Search friends...]      │
├──────────────────────────┤
│ Online Friends:          │
│ ✓ Friend1 (In Queue)     │
│ ✓ Friend2 (In Game)      │
│ ○ Friend3 (Offline)      │
├──────────────────────────┤
│ Conversations:           │
│ [Friend1]                │
│ [Friend2]                │
│ [Friend3]                │
├──────────────────────────┤
│ [Friend1 conversation]   │
│ Friend1: Hey!            │
│ You: Hi there!           │
│ Friend1: Want to queue?  │
│                          │
│ [Type message...]   [⏎]  │
└──────────────────────────┘
```

**Features:**
- Friend list with status indicators
- Multiple conversation tabs
- Pop-out window support
- Notification badges
- Search/filter functionality

---

## 4. Input System

### 4.1 Text Input Field

**Specifications:**
- **Character Limit:** 256 (team/all), 500 (whisper)
- **Input Method:** Keyboard text entry
- **Paste Support:** Yes (Ctrl+V)
- **History:** Up/Down arrows cycle through recent messages
- **Auto-complete:** Tab key for names and commands

### 4.2 Focus Management

**Focus Behavior:**
```
Enter Key:
  - If chat unfocused: Focus chat, clear input
  - If chat focused: Send message
  
Escape Key:
  - If chat focused: Unfocus chat, clear input
  - If chat unfocused: No action
  
Tab Key:
  - If chat focused: Auto-complete name/command
  - If chat unfocused: No action
```

### 4.3 Auto-Complete System

**Triggers:**
- `/` - Command auto-complete
- `@` - Player name mention
- `/w ` - Whisper target name

**Implementation:**
```c
typedef struct ChatAutoComplete {
    char prefix[32];           // Current prefix being completed
    char** suggestions;        // Array of suggestions
    uint32_t suggestion_count;
    uint32_t current_index;    // Currently highlighted suggestion
} ChatAutoComplete;
```

**Example:**
```
User types: "/t"
Suggestions: /team, /tell, /toggle
User presses Tab: "/team "

User types: "@Pla"
Suggestions: @Player1, @Player2, @PlayersUnited
User presses Tab: "@Player1 "
```

### 4.4 Chat History

**Specifications:**
- **Storage:** In-memory circular buffer
- **Capacity:** Last 50 messages per session
- **Navigation:** Up/Down arrows
- **Persistence:** Session-only (cleared on disconnect)

**Implementation:**
```c
typedef struct ChatHistory {
    char** messages;           // Circular buffer of messages
    uint32_t capacity;
    uint32_t count;
    uint32_t current_index;    // Current position in history
} ChatHistory;
```

---

## 5. Commands

### 5.1 Chat Commands

**Command Format:** `/command [args]`

| Command | Usage | Description |
|---------|-------|-------------|
| `/all` | `/all [message]` | Send to all chat |
| `/team` | `/team [message]` | Send to team chat |
| `/w` | `/w [name] [msg]` | Whisper to player |
| `/msg` | `/msg [name] [msg]` | Alias for /w |
| `/r` | `/r [message]` | Reply to last whisper |
| `/mute` | `/mute [name]` | Mute player |
| `/unmute` | `/unmute [name]` | Unmute player |
| `/ignore` | `/ignore [name]` | Permanent ignore |
| `/unignore` | `/unignore [name]` | Remove from ignore list |
| `/clear` | `/clear` | Clear chat history |
| `/help` | `/help [command]` | Show command help |
| `/emote` | `/emote [name]` | Send emote action |

### 5.2 Command Processing

**Implementation:**
```c
typedef struct ChatCommand {
    char name[32];
    char description[256];
    bool (*execute)(const char* args);
} ChatCommand;

typedef struct ChatCommandRegistry {
    ChatCommand* commands;
    uint32_t count;
    uint32_t capacity;
} ChatCommandRegistry;
```

**Processing Flow:**
```
1. User types message
2. Check if starts with '/'
3. Parse command name and arguments
4. Look up in command registry
5. Execute command handler
6. Display result or error
```

---

## 6. Message Rendering

### 6.1 Message Format

**Standard Format:**
```
[HH:MM] PlayerName: Message text
```

**System Message Format:**
```
[HH:MM] * System event description
```

**Whisper Format:**
```
[HH:MM] [Whisper] PlayerName: Message text
```

### 6.2 Username Formatting

**Color Coding by Team:**
- **Team Chat:** Green (#00FF00) for teammates
- **All Chat:** White (#FFFFFF) for all players
- **Whisper:** Cyan (#00FFFF) for sender/recipient
- **System:** Yellow (#FFFF00) for system messages

**Implementation:**
```c
typedef struct ChatMessageStyle {
    uint32_t username_color;   // RGBA color
    uint32_t text_color;
    uint32_t timestamp_color;
    bool bold_username;
    bool italic_text;
} ChatMessageStyle;
```

### 6.3 Text Wrapping

**Specifications:**
- **Wrap Width:** Panel width - 20px (padding)
- **Line Height:** 16px (configurable)
- **Word Wrap:** Break at word boundaries
- **Overflow:** Ellipsis (...) for very long words

### 6.4 Special Features

#### **Mention Highlighting**
- Detect `@PlayerName` patterns
- Highlight in bright color
- Optional sound notification
- Scroll to mention if off-screen

#### **Link Detection**
- Detect URLs (http://, https://)
- Render as clickable links
- Underline and color (blue)
- Open in browser on click

#### **Emoji Support (Optional)**
- Support Unicode emoji
- Custom emoji pack (optional)
- Emoji picker (optional)

---

## 7. Notification System

### 7.1 New Message Indicators

**In-Game:**
- **Unread Badge:** Number badge on chat panel
- **Flash:** Panel flashes when new message arrives
- **Sound:** Optional notification sound
- **Toast:** Brief popup for whispers

**Launcher:**
- **Conversation Badge:** Unread count on tab
- **Friend Indicator:** Dot next to friend name
- **System Tray:** Notification in taskbar (optional)

### 7.2 Notification Types

```c
typedef enum ChatNotificationType {
    NOTIF_NEW_MESSAGE,         // New message in channel
    NOTIF_MENTION,             // Player mentioned you
    NOTIF_WHISPER,             // New whisper received
    NOTIF_FRIEND_ONLINE,       // Friend came online
    NOTIF_PARTY_INVITE,        // Party invitation
    NOTIF_FRIEND_REQUEST,      // Friend request received
} ChatNotificationType;
```

### 7.3 Notification Settings

**Configurable Per Channel:**
- Enable/disable notifications
- Sound on/off
- Toast popup on/off
- Flash on/off
- Taskbar notification on/off

**Implementation:**
```c
typedef struct ChatNotificationSettings {
    bool enabled;
    bool play_sound;
    bool show_toast;
    bool flash_panel;
    bool taskbar_notification;
    char sound_file[256];      // Path to notification sound
} ChatNotificationSettings;
```

---

## 8. Moderation Features

### 8.1 Mute System

**Mute Individual Player:**
```
/mute PlayerName
- Hides all messages from PlayerName
- Stored in local mute list
- Persists across sessions
- Can be unmuted with /unmute
```

**Mute All Chat:**
```
- Toggle button in chat settings
- Hides all non-team messages
- Useful for competitive focus
- Persists until toggled off
```

**Implementation:**
```c
typedef struct ChatMuteList {
    char** muted_players;      // Array of muted player names
    uint32_t count;
    uint32_t capacity;
} ChatMuteList;

bool chat_is_muted(ChatMuteList* list, const char* player_name);
void chat_mute_player(ChatMuteList* list, const char* player_name);
void chat_unmute_player(ChatMuteList* list, const char* player_name);
```

### 8.2 Report System

**Report Dialog:**
- Triggered by right-click on message or player
- Reason selection (spam, harassment, hate speech, etc.)
- Optional comment field
- Submit to moderation queue

**Implementation:**
```c
typedef enum ReportReason {
    REPORT_SPAM,
    REPORT_HARASSMENT,
    REPORT_HATE_SPEECH,
    REPORT_PROFANITY,
    REPORT_CHEATING,
    REPORT_OTHER,
} ReportReason;

typedef struct ChatReport {
    uint64_t reporter_id;
    uint64_t reported_id;
    char reported_name[64];
    ReportReason reason;
    char comment[512];
    time_t timestamp;
} ChatReport;
```

### 8.3 Profanity Filter

**Features:**
- Optional word filtering
- Configurable word list
- Replacement character (*, #, etc.)
- Bypass for trusted players (optional)

**Implementation:**
```c
typedef struct ProfanityFilter {
    char** forbidden_words;
    uint32_t word_count;
    char replacement_char;
    bool enabled;
} ProfanityFilter;

char* filter_message(ProfanityFilter* filter, const char* message);
```

### 8.4 Rate Limiting

**Anti-Spam Protection:**
- Max 5 messages per 10 seconds per player
- Max 3 identical messages per minute
- Cooldown: 2 seconds between messages
- Violation: Temporary mute (30 seconds)

**Implementation:**
```c
typedef struct ChatRateLimit {
    uint32_t message_count;
    time_t window_start;
    time_t last_message_time;
    char last_message[256];
    uint32_t identical_count;
} ChatRateLimit;

bool chat_check_rate_limit(ChatRateLimit* limit);
void chat_update_rate_limit(ChatRateLimit* limit);
```

---

## 9. Chat History

### 9.1 In-Memory History

**Specifications:**
- **Storage:** Circular buffer per channel
- **Capacity:** 100 messages per channel
- **Lifetime:** Session duration
- **Scrollback:** Full history accessible via scroll

**Implementation:**
```c
typedef struct ChatMessage {
    uint64_t message_id;
    uint64_t sender_id;
    char sender_name[64];
    char content[500];
    time_t timestamp;
    ChatMessageType type;
    bool is_system_message;
    uint32_t color;            // Message color
} ChatMessage;

typedef struct ChatHistory {
    ChatMessage* messages;     // Circular buffer
    uint32_t capacity;
    uint32_t count;
    uint32_t write_index;      // Next write position
} ChatHistory;
```

### 9.2 Scrollback Functionality

**Features:**
- Scroll wheel to navigate history
- Page Up/Down for faster scrolling
- Jump to bottom (latest messages)
- Jump to top (oldest messages)
- Search history (optional)

**Implementation:**
```c
typedef struct ChatScrollback {
    uint32_t first_visible_index;  // Index of first visible message
    uint32_t visible_count;        // Number of visible messages
    bool at_bottom;                // True if showing latest messages
} ChatScrollback;

void chat_scroll_up(ChatScrollback* sb, uint32_t lines);
void chat_scroll_down(ChatScrollback* sb, uint32_t lines);
void chat_scroll_to_bottom(ChatScrollback* sb);
```

### 9.3 Clear History

**Command:**
```
/clear
- Clears current channel history
- Confirmation dialog (optional)
- Cannot be undone
```

### 9.4 Search History (Optional)

**Features:**
- Search by player name
- Search by keyword
- Filter by date range
- Filter by message type

---

## 10. Network Protocol

### 10.1 Chat Message Packet Structure

**Packet Type:** `PACKET_CHAT_MESSAGE`

```c
typedef struct PacketChatMessage {
    PacketHeader header;
    uint64_t message_id;       // Unique message ID
    uint64_t channel_id;       // Channel identifier
    uint64_t sender_id;        // Sender player ID
    char sender_name[64];
    ChatChannelType channel_type;
    ChatMessageType message_type;
    char content[500];
    time_t timestamp;
    uint8_t flags;             // System message, mention, etc.
} PacketChatMessage;
```

**Packet Size:** ~700 bytes

### 10.2 Channel Types (Network)

```c
typedef enum ChatChannelType {
    CHAT_CHANNEL_TEAM,         // Team chat (in-game)
    CHAT_CHANNEL_ALL,          // All chat (in-game)
    CHAT_CHANNEL_PARTY,        // Party chat (lobby)
    CHAT_CHANNEL_WHISPER,      // Direct message
    CHAT_CHANNEL_CLUB,         // Club/guild chat
} ChatChannelType;
```

### 10.3 Server Relay vs P2P

**Architecture:** Server Relay (recommended)

**Advantages:**
- Centralized message ordering
- Easier moderation
- Reliable delivery
- Message persistence
- Spam filtering

**Flow:**
```
Client A → Server → All Clients in Channel
```

**Implementation:**
```c
// Server-side
void server_handle_chat_message(GameServer* server, int client_id, 
                                PacketChatMessage* msg);
void server_broadcast_chat_message(GameServer* server, 
                                   PacketChatMessage* msg);

// Client-side
void net_client_send_chat_message(NetClient* client, 
                                  const char* content,
                                  ChatChannelType channel);
void net_client_receive_chat_message(NetClient* client, 
                                     PacketChatMessage* msg);
```

### 10.4 Message Ordering

**Guarantees:**
- Messages ordered by server timestamp
- Sequence numbers for reliability
- Acknowledgment of receipt (optional)

**Implementation:**
```c
typedef struct ChatMessageSequence {
    uint32_t sequence;         // Message sequence number
    uint32_t ack;              // Acknowledged up to this sequence
    time_t server_timestamp;   // Server-side timestamp
} ChatMessageSequence;
```

### 10.5 Handling Disconnections

**Behavior:**
- Unsent messages stored locally
- Retry on reconnection
- Notify user of failed sends
- Clear unsent queue on disconnect

**Implementation:**
```c
typedef struct ChatPendingMessage {
    PacketChatMessage msg;
    time_t created_at;
    uint32_t retry_count;
    bool failed;
} ChatPendingMessage;

typedef struct ChatPendingQueue {
    ChatPendingMessage* messages;
    uint32_t count;
    uint32_t capacity;
} ChatPendingQueue;
```

---

## 11. UI Implementation

### 11.1 Chat Panel Component

**File Structure:**
```
src/client/ui/
├── chat/
│   ├── chat_panel.h
│   ├── chat_panel.c
│   ├── chat_input.h
│   ├── chat_input.c
│   ├── chat_renderer.h
│   ├── chat_renderer.c
│   ├── chat_history.h
│   ├── chat_history.c
│   └── chat_commands.h
│   └── chat_commands.c
```

### 11.2 Core Data Structures

**Chat Panel State:**
```c
typedef struct ChatPanel {
    // Rendering
    float x, y;                // Position
    float width, height;       // Size
    float alpha;               // Transparency (0-1)
    bool visible;
    bool minimized;
    
    // Content
    ChatHistory history;
    ChatScrollback scrollback;
    ChatInputState input;
    
    // Channel
    ChatChannelType current_channel;
    ChatChannelType* available_channels;
    uint32_t channel_count;
    
    // Notifications
    uint32_t unread_count;
    bool has_mention;
    
    // Settings
    ChatPanelSettings settings;
    ChatMuteList mute_list;
    
    // Timing
    double fade_timer;         // For fade-out effect
    double last_message_time;
} ChatPanel;
```

**Chat Input State:**
```c
typedef struct ChatInputState {
    char buffer[512];          // Input text buffer
    uint32_t cursor_pos;       // Cursor position
    uint32_t selection_start;
    uint32_t selection_end;
    
    // History navigation
    ChatHistory* history;
    int32_t history_index;     // -1 = current input
    
    // Auto-complete
    ChatAutoComplete autocomplete;
    
    // Focus
    bool focused;
    double focus_time;
} ChatInputState;
```

### 11.3 Rendering Pipeline

**Render Order:**
```
1. Background panel (semi-transparent)
2. Header bar
3. Message list (with scrollbar)
4. Input field
5. Notifications (toasts, badges)
```

**Implementation:**
```c
void chat_panel_render(ChatPanel* panel, Renderer* renderer);
void chat_panel_render_background(ChatPanel* panel, Renderer* renderer);
void chat_panel_render_header(ChatPanel* panel, Renderer* renderer);
void chat_panel_render_messages(ChatPanel* panel, Renderer* renderer);
void chat_panel_render_input(ChatPanel* panel, Renderer* renderer);
void chat_panel_render_scrollbar(ChatPanel* panel, Renderer* renderer);
```

### 11.4 Input Handling

**Keyboard Input:**
```c
void chat_panel_handle_key(ChatPanel* panel, int key, int action, int mods);
void chat_input_handle_char(ChatInputState* input, unsigned int codepoint);
void chat_input_handle_backspace(ChatInputState* input);
void chat_input_handle_delete(ChatInputState* input);
void chat_input_handle_enter(ChatInputState* input);
void chat_input_handle_escape(ChatInputState* input);
void chat_input_handle_tab(ChatInputState* input);
void chat_input_handle_arrow(ChatInputState* input, int direction);
```

**Mouse Input:**
```c
void chat_panel_handle_mouse_click(ChatPanel* panel, double x, double y, int button);
void chat_panel_handle_mouse_scroll(ChatPanel* panel, double x, double y);
void chat_panel_handle_mouse_move(ChatPanel* panel, double x, double y);
```

---

## 12. Code Structures

### 12.1 Chat Message Structure

```c
typedef struct ChatMessage {
    uint64_t message_id;       // Unique identifier
    uint64_t channel_id;       // Channel this message belongs to
    uint64_t sender_id;        // Player ID of sender
    char sender_name[64];      // Display name
    char content[500];         // Message text
    time_t timestamp;          // Server timestamp
    ChatMessageType type;      // Text, system, event, etc.
    ChatChannelType channel;   // Which channel
    uint32_t color;            // RGBA color for rendering
    bool is_system_message;    // True for system messages
    bool is_mention;           // True if mentions current player
    uint8_t flags;             // Additional flags
} ChatMessage;
```

### 12.2 Chat Channel Structure

```c
typedef struct ChatChannel {
    uint64_t channel_id;
    ChatChannelType type;
    char name[64];
    
    // Message storage
    ChatHistory history;
    
    // Participants
    uint64_t* participant_ids;
    uint32_t participant_count;
    
    // Settings
    bool enabled;
    bool muted;
    ChatNotificationSettings notifications;
    
    // Metadata
    time_t created_at;
    time_t last_message_time;
} ChatChannel;
```

### 12.3 Chat History Structure

```c
typedef struct ChatHistory {
    ChatMessage* messages;     // Circular buffer
    uint32_t capacity;         // Max messages
    uint32_t count;            // Current count
    uint32_t write_index;      // Next write position
    
    // Search/filtering
    char search_query[256];
    ChatMessage** search_results;
    uint32_t search_count;
} ChatHistory;
```

### 12.4 Chat Input State Structure

```c
typedef struct ChatInputState {
    char buffer[512];          // Input text
    uint32_t length;           // Current length
    uint32_t cursor_pos;       // Cursor position
    
    // Selection
    uint32_t selection_start;
    uint32_t selection_end;
    
    // History
    char** history;            // Previous messages
    uint32_t history_count;
    int32_t history_index;     // -1 = current input
    
    // Auto-complete
    char** suggestions;
    uint32_t suggestion_count;
    uint32_t current_suggestion;
    
    // State
    bool focused;
    bool modified;
    double focus_time;
} ChatInputState;
```

### 12.5 Mute List Structure

```c
typedef struct ChatMuteList {
    char** muted_players;      // Array of player names
    uint32_t count;
    uint32_t capacity;
    
    // Metadata
    time_t* mute_times;        // When each player was muted
    bool* permanent;           // Permanent vs temporary mute
} ChatMuteList;
```

### 12.6 Chat Panel State Structure

```c
typedef struct ChatPanel {
    // Geometry
    float x, y;
    float width, height;
    bool resizable;
    
    // Visibility
    bool visible;
    bool minimized;
    float alpha;               // Transparency
    
    // Content
    ChatChannel* channels;
    uint32_t channel_count;
    uint32_t current_channel_index;
    
    // Input
    ChatInputState input;
    
    // History
    ChatHistory history;
    ChatScrollback scrollback;
    
    // Notifications
    uint32_t unread_count;
    bool has_mention;
    
    // Moderation
    ChatMuteList mute_list;
    ProfanityFilter profanity_filter;
    ChatRateLimit rate_limit;
    
    // Settings
    ChatPanelSettings settings;
    
    // Timing
    double fade_timer;
    double last_message_time;
    double last_input_time;
} ChatPanel;
```

---

## 13. Integration Points

### 13.1 Friends System Integration

**Features:**
- Quick whisper from friend list
- Friend online/offline notifications
- Friend request handling
- Friend status display (in queue, in game, offline)

**API:**
```c
void chat_whisper_friend(ChatPanel* panel, uint64_t friend_id);
void chat_on_friend_online(ChatPanel* panel, uint64_t friend_id);
void chat_on_friend_offline(ChatPanel* panel, uint64_t friend_id);
void chat_on_friend_request(ChatPanel* panel, uint64_t requester_id);
```

### 13.2 Party System Integration

**Features:**
- Party chat channel
- Party member list in chat
- Ready status notifications
- Champion selection notifications

**API:**
```c
void chat_create_party_channel(ChatPanel* panel, uint64_t party_id);
void chat_add_party_member(ChatPanel* panel, uint64_t member_id);
void chat_remove_party_member(ChatPanel* panel, uint64_t member_id);
void chat_on_party_member_ready(ChatPanel* panel, uint64_t member_id);
void chat_on_champion_selected(ChatPanel* panel, uint64_t member_id, 
                               const char* champion_name);
```

### 13.3 In-Game Events Integration

**Kill Feed:**
```c
void chat_add_kill_event(ChatPanel* panel, 
                         const char* killer_name,
                         const char* victim_name,
                         const char* ability_name);
```

**Objective Events:**
```c
void chat_add_objective_event(ChatPanel* panel,
                              const char* team_name,
                              const char* objective_name);
```

**System Events:**
```c
void chat_add_system_event(ChatPanel* panel,
                           SystemMessageType type,
                           const char* description);
```

### 13.4 Steam Overlay Integration

**Features:**
- Detect Steam overlay active
- Adjust chat rendering when overlay visible
- Handle input focus conflicts

**API:**
```c
bool chat_is_steam_overlay_active(void);
void chat_on_steam_overlay_activated(ChatPanel* panel);
void chat_on_steam_overlay_deactivated(ChatPanel* panel);
```

---

## 14. Accessibility

### 14.1 Font Size Options

**Configurable Sizes:**
- Small: 10px
- Normal: 12px (default)
- Large: 14px
- Extra Large: 16px

**Implementation:**
```c
typedef enum ChatFontSize {
    CHAT_FONT_SMALL,
    CHAT_FONT_NORMAL,
    CHAT_FONT_LARGE,
    CHAT_FONT_XLARGE,
} ChatFontSize;

void chat_set_font_size(ChatPanel* panel, ChatFontSize size);
```

### 14.2 High Contrast Mode

**Features:**
- Increased color contrast
- Thicker text rendering
- Larger UI elements
- Simplified styling

**Implementation:**
```c
typedef struct ChatHighContrastTheme {
    uint32_t background_color;
    uint32_t text_color;
    uint32_t highlight_color;
    uint32_t border_color;
    float border_width;
} ChatHighContrastTheme;

void chat_enable_high_contrast(ChatPanel* panel);
```

### 14.3 Screen Reader Support (Optional)

**Features:**
- Alt text for UI elements
- Message announcements
- Command descriptions
- Status updates

**Implementation:**
```c
void chat_announce_message(const char* message);
void chat_announce_status(const char* status);
void chat_set_element_label(const char* element_id, const char* label);
```

### 14.4 Colorblind-Friendly Colors

**Color Palette:**
- Team Chat: Blue (#0173B2) instead of green
- All Chat: Gray (#999999) instead of white
- Whisper: Purple (#DE8F05) instead of cyan
- System: Orange (#CC78BC) instead of yellow
- Mentions: Red (#CA9161) for high visibility

**Implementation:**
```c
typedef enum ChatColorScheme {
    CHAT_COLORS_NORMAL,
    CHAT_COLORS_DEUTERANOPIA,  // Red-green colorblind
    CHAT_COLORS_PROTANOPIA,    // Red-green colorblind
    CHAT_COLORS_TRITANOPIA,    // Blue-yellow colorblind
} ChatColorScheme;

void chat_set_color_scheme(ChatPanel* panel, ChatColorScheme scheme);
```

---

## 15. Performance

### 15.1 Message Pooling

**Objective:** Reduce allocation overhead

**Implementation:**
```c
typedef struct ChatMessagePool {
    ChatMessage* messages;     // Pre-allocated pool
    uint32_t capacity;
    uint32_t available;        // Available messages
    uint32_t* free_indices;    // Stack of free indices
} ChatMessagePool;

ChatMessage* chat_message_pool_alloc(ChatMessagePool* pool);
void chat_message_pool_free(ChatMessagePool* pool, ChatMessage* msg);
```

### 15.2 Efficient Text Rendering

**Techniques:**
- Glyph caching
- Batch text rendering
- Lazy layout calculation
- Dirty flag optimization

**Implementation:**
```c
typedef struct ChatTextCache {
    char* text;
    uint32_t* glyph_indices;
    float* glyph_positions;
    uint32_t glyph_count;
    bool dirty;
} ChatTextCache;

void chat_text_cache_update(ChatTextCache* cache, const char* text);
void chat_text_cache_render(ChatTextCache* cache, Renderer* renderer);
```

### 15.3 Batched Updates

**Optimization:**
- Batch message additions
- Batch rendering calls
- Defer layout calculations

**Implementation:**
```c
typedef struct ChatUpdateBatch {
    ChatMessage** messages;
    uint32_t count;
    bool layout_dirty;
} ChatUpdateBatch;

void chat_panel_begin_batch(ChatPanel* panel);
void chat_panel_add_message_batch(ChatPanel* panel, ChatMessage* msg);
void chat_panel_end_batch(ChatPanel* panel);
```

### 15.4 Memory Limits

**Constraints:**
- Max 100 messages per channel in memory
- Max 50 channels per panel
- Max 1000 muted players
- Max 500 pending messages

**Monitoring:**
```c
typedef struct ChatMemoryStats {
    uint32_t total_messages;
    uint32_t total_channels;
    uint32_t muted_players;
    uint32_t pending_messages;
    size_t memory_used;
} ChatMemoryStats;

ChatMemoryStats chat_get_memory_stats(ChatPanel* panel);
```

---

## 16. Implementation Roadmap

### Phase 1: Core Infrastructure (Weeks 1-2)

**Deliverables:**
- [ ] Chat message structures and enums
- [ ] Chat history circular buffer
- [ ] Basic packet definitions
- [ ] Network protocol handlers

**Files:**
- `src/arena/network/chat_protocol.h`
- `src/arena/chat/chat_types.h`
- `src/arena/chat/chat_history.h/c`

### Phase 2: Server-Side Chat (Weeks 3-4)

**Deliverables:**
- [ ] Server chat message handler
- [ ] Message relay system
- [ ] Channel management
- [ ] Rate limiting and spam detection

**Files:**
- `src/server/chat_server.h/c`
- `src/server/chat_channels.h/c`
- `src/server/chat_moderation.h/c`

### Phase 3: Client-Side Chat (Weeks 5-6)

**Deliverables:**
- [ ] Chat panel UI component
- [ ] Message input system
- [ ] Chat history management
- [ ] Command processing

**Files:**
- `src/client/ui/chat/chat_panel.h/c`
- `src/client/ui/chat/chat_input.h/c`
- `src/client/ui/chat/chat_commands.h/c`

### Phase 4: Rendering and Display (Weeks 7-8)

**Deliverables:**
- [ ] Message rendering
- [ ] Text layout and wrapping
- [ ] Scrollbar implementation
- [ ] Notification system

**Files:**
- `src/client/ui/chat/chat_renderer.h/c`
- `src/client/ui/chat/chat_notifications.h/c`

### Phase 5: Features and Polish (Weeks 9-10)

**Deliverables:**
- [ ] Mute/ignore system
- [ ] Report functionality
- [ ] Profanity filter
- [ ] Auto-complete system
- [ ] Integration with friends/party systems
- [ ] Testing and bug fixes

**Files:**
- `src/client/ui/chat/chat_moderation.h/c`
- `src/client/ui/chat/chat_autocomplete.h/c`

### Testing Strategy

**Unit Tests:**
- Message parsing and validation
- History buffer operations
- Command parsing
- Rate limiting logic

**Integration Tests:**
- Server-client message flow
- Channel switching
- Mute/unmute functionality
- Command execution

**UI Tests:**
- Input handling
- Rendering correctness
- Notification display
- Scrolling behavior

---

## Appendix A: Configuration

### Default Settings

```c
typedef struct ChatPanelSettings {
    // Display
    float panel_width;         // 400px
    float panel_height;        // 200px
    float panel_alpha;         // 0.7
    ChatFontSize font_size;    // NORMAL
    
    // Behavior
    bool fade_when_inactive;   // true
    float fade_timeout;        // 10.0 seconds
    uint32_t max_visible_messages;  // 50
    
    // Notifications
    bool notify_new_message;   // true
    bool notify_mention;       // true
    bool notify_whisper;       // true
    bool play_notification_sound;  // true
    
    // Moderation
    bool enable_profanity_filter;  // false
    bool mute_all_chat;        // false
    
    // Accessibility
    ChatColorScheme color_scheme;  // NORMAL
    bool high_contrast;        // false
} ChatPanelSettings;
```

### Default Hotkeys

| Action | Hotkey |
|--------|--------|
| Focus Chat | Enter |
| Send Message | Enter (when focused) |
| Unfocus Chat | Escape |
| Team Chat | Enter |
| All Chat | Shift+Enter |
| Whisper | /w |
| Auto-complete | Tab |
| Message History Up | Up Arrow |
| Message History Down | Down Arrow |
| Scroll Up | Scroll Wheel Up |
| Scroll Down | Scroll Wheel Down |

---

## Appendix B: Error Handling

### Network Errors

```c
typedef enum ChatNetworkError {
    CHAT_ERR_NONE,
    CHAT_ERR_SEND_FAILED,      // Failed to send message
    CHAT_ERR_TIMEOUT,          // Server didn't respond
    CHAT_ERR_DISCONNECTED,     // Lost connection
    CHAT_ERR_RATE_LIMITED,     // Too many messages
    CHAT_ERR_INVALID_CHANNEL,  // Channel doesn't exist
    CHAT_ERR_PERMISSION_DENIED,// Not allowed in channel
} ChatNetworkError;

void chat_on_network_error(ChatPanel* panel, ChatNetworkError error);
```

### Validation

```c
bool chat_validate_message(const char* message, uint32_t max_length);
bool chat_validate_player_name(const char* name);
bool chat_validate_command(const char* command);
```

---

## Appendix C: Future Enhancements

### Planned Features

1. **Rich Text Formatting**
   - Bold, italic, underline
   - Color codes
   - Code blocks

2. **Voice Chat Integration**
   - Voice channel indicators
   - Push-to-talk status
   - Voice chat invitations

3. **Emote System**
   - Custom emotes
   - Emote picker
   - Emote animations

4. **Chat Reactions**
   - Emoji reactions to messages
   - Reaction counts
   - Reaction animations

5. **Message Editing**
   - Edit sent messages
   - Edit history
   - Deletion with reason

6. **Advanced Moderation**
   - Automated moderation AI
   - Keyword filtering
   - Toxicity detection

7. **Chat Themes**
   - Custom color schemes
   - Font customization
   - Layout presets

8. **Message Pinning**
   - Pin important messages
   - Pinned message list
   - Pin notifications

---

## Appendix D: References

### Related Documentation
- `TECHNICAL_SPEC.md` - Engine architecture
- `ARCHITECTURE_DECISIONS.md` - Design patterns
- `TOOLS_LAUNCHER_DESIGN.md` - Launcher UI framework
- `3D_PROPOSAL_EXPANSION.md` - Chat protocol section

### External References
- Dear ImGui documentation (for launcher chat)
- Vulkan text rendering best practices
- Network protocol design patterns
- MOBA chat system analysis (League of Legends, Dota 2)

---

**Document Version:** 1.0  
**Last Updated:** 2026-03-08  
**Status:** Ready for Implementation Planning

