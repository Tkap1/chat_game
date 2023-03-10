
global constexpr char* c_python_path = "C:/Users/PakT/AppData/Local/Programs/Python/Python37-32/python.exe";

global constexpr int c_max_health = 100;

global constexpr int c_cells_right = 32;

// @Note(tkap, 10/03/2023): Amount of chatters alive has to be lower than this in order to display some things,
// like chatter name and health bar
global constexpr int c_chatters_alive_threshold = 40;
global constexpr float c_shoot_delay = 0.5f;
global constexpr float c_message_duration = 3.0f;

global constexpr s_v2 c_origin_topleft = v2(1.0f, -1.0f);
global constexpr s_v2 c_origin_bottomleft = v2(1.0f, 1.0f);
global constexpr s_v2 c_origin_center = v2(0, 0);
global constexpr s_v2 c_chatter_size = v2(64);
global constexpr s_v2 c_projectile_size = v2(32);

global constexpr s_v2 c_base_res = v2(1280, 720);

global constexpr char* c_chat_messages_file_path = "chat_messages.txt";
global constexpr char* c_chatters_file_path = "chatters.txt";


global constexpr int c_projectile_damage = 5;

// @Note(tkap, 10/03/2023): Skill data
global constexpr float c_nova_cooldown = 10;
global constexpr int c_nova_projectile_count = 50;
global constexpr int c_nova_damage = 3;

global constexpr float c_shield_cooldown = 20;
global constexpr float c_shield_duration = 5;

