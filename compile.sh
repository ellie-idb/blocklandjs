i686-w64-mingw32-g++ -shared -L./ "./dllmain.cpp" "./Torque.cpp" "./duktape.c" "./libmainlib.a" -lws2_32 -liphlpapi -luserenv \
-o "BlocklandJS.dll" \
-I./lib/dukluv/include -I./lib/uv/include -I./ \
-static-libgcc \
-static-libstdc++ \
-Wl,-Bstatic \
-lstdc++ \
-lpsapi \
-lpthread \
-Wl,-Bdynamic \
-w 

cp BlocklandJS.dll ~/BlocklandPortable/BlocklandJS.dll
