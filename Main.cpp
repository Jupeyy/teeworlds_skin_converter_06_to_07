#include "Converter.h"

#include <string>
#include <stdio.h>
#include <iostream>
#include <vector>
#include "helper_functions.h"
#include <string.h>

int main(int ArgCount, char* pArgs[]) {
	std::vector<std::string> Args;
	for(int i = 0; i < ArgCount; ++i) {
		Args.push_back(pArgs[i]);
	}

	for(size_t i = 1; i < Args.size(); ++i) {
		CConverter Conv;
		std::cout << Args[i] << std::endl;
		std::vector<directory_item_info> ItemInfos;
		directory_items(Args[i].c_str(), ItemInfos, NULL);

		for(size_t n = 0; n < ItemInfos.size(); ++n) {
			if(!ItemInfos[n].is_dir) {
				std::string Name = ItemInfos[n].item.file.name;
				if(Name.size() > 4) {
					std::string NameShort = std::string(Name.c_str(), Name.size() - 4);
					std::cout << "file: " << Name << std::endl;
					Conv.Convert((Args[i] + "/" + Name).c_str(), NameShort.c_str());
				}
			}
		}
	}

	return 0;
}
