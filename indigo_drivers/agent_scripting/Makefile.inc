indigo_agent_scripting.o: boot.js.dat

boot.js.dat: boot.js
		hexdump -v -e '1/1 "0x%02x, "' boot.js >boot.js.dat
