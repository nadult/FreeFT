cd data_embed/fonts
xxd -i liberation_16_0.png > ../../build/res_embedded.cpp
xxd -i liberation_24_0.png >> ../../build/res_embedded.cpp
xxd -i liberation_32_0.png >> ../../build/res_embedded.cpp
xxd -i liberation_48_0.png >> ../../build/res_embedded.cpp
xxd -i transformers_20_0.png >> ../../build/res_embedded.cpp
xxd -i transformers_30_0.png >> ../../build/res_embedded.cpp
xxd -i transformers_48_0.png >> ../../build/res_embedded.cpp
cd ..
xxd -i loading_bar.png >> ../build/res_embedded.cpp
xxd -i icons.png >> ../build/res_embedded.cpp
