
import socket
import secret
import time
import config import target_channel

twitch = None

def send(to_send):
	twitch.send(bytes(to_send + "\n", encoding="utf-8"))

def main():
	global twitch

	# @Note(tkap, 01/12/2022): Try to connect until it works. Twitch likes to fail every now and then for some reason
	while True:
		twitch = socket.create_connection(("irc.chat.twitch.tv", 6667))
		send("PASS oauth:%s" % secret.accesss_token)
		send("NICK %s" % target_channel)
		try:
			print(twitch.recv(8192))
			break

		except ConnectionResetError:
			print("Failed to connect to twitch, retrying...")
			twitch.close()

	send("JOIN #%s" % target_channel)
	print(twitch.recv(8192))

	# @Note(tkap, 31/10/2022): Handle chat messages
	while True:
		data = twitch.recv(8192)
		if not data: break
		# print(data)
		data = data.decode()
		data = data.splitlines()[0]
		print(data)

		if "PRIVMSG" in data:
			exclamation_index = data.find("!")
			assert exclamation_index != 1
			user = data[1 : exclamation_index]
			colon_index = data[1:].find(":")
			if colon_index != -1:
				msg = data[colon_index + 2:]
				msg = msg.replace("\r", "")
				msg = msg.strip()

				# @Note(tkap, 10/03/2023): Try append message to file
				while True:
					try:
						with open("chat_messages.txt", "a") as fo:
							fo.write(f"{user}:{msg}\n")
						break
					except IOError:
						time.sleep(0.25)

	twitch.close()


if __name__ == "__main__":
	main()