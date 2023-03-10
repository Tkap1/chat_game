

import requests
from config import target_channel

def get_non_bot_chatters():

	chatters = []
	known_bots = [
		"streamlabs",
		"aliengathering",
		"anotherttvviewer",
		"chatstats_",
		"commanderroot",
		"discordstreamercommunity",
		"smallstreamersdccommunity",
		"01ella",
		"0ax2",
		"aliceydra",
		"audycia",
		"iizzybeth",
		"ssgdcserver",
		# "explodedsoda", not sure if this is a bot or not
	]
	r = requests.get("https://tmi.twitch.tv/group/user/%s/chatters" % target_channel)

	if r.status_code != 200:
		return []

	data = r.json()["chatters"]

	queries = [
		"broadcaster",
		"vips",
		"moderators",
		"staff",
		"admins",
		"global_mods",
		"viewers",
	]

	for q in queries:
		for user in data[q]:
			if user not in known_bots:
				chatters.append(user)

	return chatters


chatters = get_non_bot_chatters()
with open("chatters.txt", "w") as fo:
	for c in chatters:
		fo.write(f"{c}\n")