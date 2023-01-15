#include "SoundboardDisplay.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <functional>
#include <Windows.h>

#include <fstream>

SoundboardDisplay::SoundboardDisplay() {
	window = boost::ui::dialog("Soundboard inputs");
	window.resize(450, 300);

	boost::ui::vbox mainBox(window);
	boost::ui::hbox firstHeader;
	boost::ui::hbox secondHeader;
	boost::ui::hbox volumeHeader;
	boost::ui::hbox thirdHeader;
	boost::ui::hbox connectionHeader;
	boost::ui::hbox body;

	connectAppButton = boost::ui::button(window, "Connect to octopus");
	connectAppInput = boost::ui::string_box(window, "ws://localhost:8000/");
	connectAppButton.on_press(std::bind(&SoundboardDisplay::connectToApp, this));
	connectAppStatus = boost::ui::label(window, "Not connected");

	boost::ui::label nameInputLabel = boost::ui::label(window, "Sound name: ");
	nameInput = boost::ui::string_box(window);
	isMusicBox = boost::ui::check_box(window, "Music ?");
	addItemButton = boost::ui::button(window, "Add hotkey");
	addItemButton.on_press(std::bind(&SoundboardDisplay::addItemCb, this));
	addItemStatus = boost::ui::label(window);

	nextMusicButton = boost::ui::button(window, "Hotkey : Next Music");
	nextMusicButton.on_press(std::bind(&SoundboardDisplay::addNextMusicInputCb, this));
	nextMusicLabel = boost::ui::label(window, "none");
	stopMusicButton = boost::ui::button(window, "Hotkey : Stop Music");
	stopMusicButton.on_press(std::bind(&SoundboardDisplay::addStopMusicInputCb, this));
	stopMusicLabel = boost::ui::label(window, "none");

	deleteItemButton = boost::ui::button(window, "Remove hotkey");
	deleteItemButton.on_press(std::bind(&SoundboardDisplay::removeItemCb, this));

	updateVolumeButton = boost::ui::button(window, "Send volume");
	updateVolumeButton.on_press(std::bind(&SoundboardDisplay::sendVolumeUpdate, this));
	volumeSlider = boost::ui::hslider(window, 75, 0, 100);
	volumeSlider.on_slide(std::bind(&SoundboardDisplay::updateVolumeLabel, this));
	volumeLabel = boost::ui::label(window, "100%");

	itemList = boost::ui::list_box(window);

	connectionHeader << connectAppInput.layout().stretch();
	connectionHeader << connectAppButton.layout().justify();
	connectionHeader << connectAppStatus.layout().vcenter();

	firstHeader << nameInputLabel.layout().vcenter();
	firstHeader << nameInput.layout().stretch();
	firstHeader << isMusicBox.layout().justify();
	firstHeader << addItemButton.layout().justify();
	firstHeader << addItemStatus.layout().vcenter();

	secondHeader << nextMusicButton;
	secondHeader << nextMusicLabel;
	secondHeader << stopMusicButton;
	secondHeader << stopMusicLabel;

	volumeHeader << updateVolumeButton.layout().justify();
	volumeHeader << volumeSlider.layout().stretch();
	volumeHeader << volumeLabel.layout().justify();

	thirdHeader << deleteItemButton.layout().justify();

	mainBox << connectionHeader.layout().justify();
	mainBox << firstHeader.layout().justify();
	mainBox << secondHeader.layout().justify();
	mainBox << volumeHeader.layout().justify();
	mainBox << thirdHeader.layout().justify();
	mainBox << itemList.layout().justify().stretch();

	load();
}

void SoundboardDisplay::connectToApp() {
	app.setUri(connectAppInput.text().string());
	connectAppStatus.text("Connecting...");
	updateDisplay();
	
	bool connected = app.connect();
	if (connected) {
		connectAppStatus.text("Connected");
		app.sendData({
			{"type", "init"},
			{"app", {
				{"type", "SoundboardDisplay"}
			}}
		});
	} else {
		connectAppStatus.text("Could not connect");
	}
	updateDisplay();
}

void SoundboardDisplay::sendSound(uint8_t vKey, void* inputDevice, bool keyPressed) {
	if (!app.isConnected() || !keyPressed)
		return;

	std::string soundName = "";
	std::string channelName = "";
	int volume = 100;

	for (Sound& sound : sounds) {
		if (sound.vKey == vKey && inputDevice == device) {
			soundName = sound.name;
			channelName = sound.is_music ? "PlayMusic" : "PlaySound";
			volume = sound.is_music ? this->volumeSlider.value() : 100;
			break;
		}
	}
	
	if (soundName.empty())
		return;

	std::string uuid = boost::lexical_cast<std::string>(boost::uuids::random_generator()());

	app.sendData({
		{"type", "broadcast"},
		{"id", uuid},
		{"channel", channelName},
		{"content", {
				{"name", soundName},
				{"volume", volume},
			}
		}
		});
}

void SoundboardDisplay::sendNextMusic(bool keyPressed) {
	if (!app.isConnected() || !keyPressed)
		return;

	std::string uuid = boost::lexical_cast<std::string>(boost::uuids::random_generator()());

	app.sendData({
		{"type", "broadcast"},
		{"id", uuid},
		{"channel", "NextMusic"},
		});
}

void SoundboardDisplay::sendStopMusic(bool keyPressed) {
	if (!app.isConnected() || !keyPressed)
		return;

	std::string uuid = boost::lexical_cast<std::string>(boost::uuids::random_generator()());

	app.sendData({
		{"type", "broadcast"},
		{"id", uuid},
		{"channel", "StopSounds"},
		});
}

void SoundboardDisplay::addSound(uint8_t vKey, void* inputDevice, bool keyPressed) {
	if (vKey != 27) {
		sounds.push_back({ vKey, nameInput.text().string(), isMusicBox.is_checked() });
		device = inputDevice;
		nameInput.clear();
	}

	nameInput.enable();
	addItemButton.enable();
	addItemStatus.text("");

	updateItemList();
	updateDisplay();
	save();

	addingInput = false;
}

void SoundboardDisplay::addNextMusicInput(uint8_t vKey, void* inputDevice, bool keyPressed) {
	if (vKey != 27) {
		nextMusicInput = { vKey, "", false };
		device = inputDevice;
	}

	nextMusicButton.enable();

	updateItemList();
	updateDisplay();
	save();

	addingNextMusicInput= false;
}

void SoundboardDisplay::addStopMusicInput(uint8_t vKey, void* inputDevice, bool keyPressed) {
	if (vKey != 27) {
		stopMusicInput = { vKey, "", false };
		device = inputDevice;
	}

	stopMusicButton.enable();

	updateItemList();
	updateDisplay();
	save();

	addingStopMusicInput = false;
}

bool SoundboardDisplay::processInput(uint8_t vKey, void* inputDevice, bool keyPressed) {
	if (addingInput) {
		app.addWork(&SoundboardDisplay::addSound, this, vKey, inputDevice, keyPressed);
		return true;
	}
	else if (addingNextMusicInput) {
		app.addWork(&SoundboardDisplay::addNextMusicInput, this, vKey, inputDevice, keyPressed);
		return true;
	}
	else if (addingStopMusicInput) {
		app.addWork(&SoundboardDisplay::addStopMusicInput, this, vKey, inputDevice, keyPressed);
		return true;
	}
	else {
		if (nextMusicInput.vKey == vKey && inputDevice == device) {
			app.addWork(&SoundboardDisplay::sendNextMusic, this, keyPressed);
			return true;

		}
		if (stopMusicInput.vKey == vKey && inputDevice == device) {
			app.addWork(&SoundboardDisplay::sendStopMusic, this, keyPressed);
			return true;
		}

		for (Sound& sound : sounds) {
			if (sound.vKey == vKey && inputDevice == device) {
				app.addWork(&SoundboardDisplay::sendSound, this, vKey, device, keyPressed);
				return true;	
			}
		}
	}
	return false;
}

void SoundboardDisplay::addItemCb() {
	if (nameInput.text().string().empty() || addingNextMusicInput || addingStopMusicInput)
		return;

	nameInput.disable();
	addItemButton.disable();
	addItemStatus.text("Enter hotkey");
	updateDisplay();

	addingInput = true;
}

void SoundboardDisplay::addNextMusicInputCb() {
	if (addingInput || addingStopMusicInput)
		return;

	nextMusicButton.disable();
	nextMusicLabel.text("Enter hotkey");
	updateDisplay();

	addingNextMusicInput = true;
}

void SoundboardDisplay::addStopMusicInputCb() {
	if (addingNextMusicInput || addingInput)
		return;

	stopMusicButton.disable();
	stopMusicLabel.text("Enter hotkey");
	updateDisplay();

	addingStopMusicInput = true;
}

void SoundboardDisplay::removeItemCb() {
	if (itemList.has_selection()) {
		auto index = itemList.selected_index();
		sounds.erase(sounds.begin() + index);

		updateItemList();
		save();
	}
}

void SoundboardDisplay::load() {
	std::ifstream f("./config.json");
	if (f.is_open()) {
		Json::json config = Json::json::parse(f);

		nextMusicInput = { config["nextMusic"]["vKey"], "", false };
		stopMusicInput = { config["stopMusic"]["vKey"], "", false };
	
		device = (void*)(int)config["device"];

		for (auto sound : config["sounds"]) {
			sounds.push_back({ sound["vKey"],  sound["name"], sound["is_music"]});
		}

		updateItemList();
		updateDisplay();
	}
}

void SoundboardDisplay::save() {
	Json::json soundsConfig = Json::json::array();
	//config.push_back
	for (Sound& sound : sounds) {
		soundsConfig.push_back({
			{"vKey", sound.vKey},
			{"name", sound.name},
			//{"device", (unsigned long)sound.device},
			{"is_music", sound.is_music}
			});
	}

	Json::json config = {
		{"nextMusic", {
			{"vKey", nextMusicInput.vKey},
			//{"device", (unsigned long) nextMusicInput.device},
		}},
		{"stopMusic", {
			{"vKey", stopMusicInput.vKey},
			//{"device", (unsigned long) stopMusicInput.device},
		}},
		{ "device" ,  (unsigned long) device },
		{"sounds", soundsConfig},
	};
	
	std::ofstream o("./config.json");
	o << std::setw(4) << config << std::endl;
}

void SoundboardDisplay::updateDisplay() {
	window.resize(window.width() + 1, window.height());
	window.resize(window.width() - 1, window.height());
}

void SoundboardDisplay::updateItemList() {
	itemList.clear();

	// Next Music
	wchar_t c[2] = { 0 };
	BYTE kb[256];
	GetKeyboardState(kb);
	MapVirtualKey(nextMusicInput.vKey, MAPVK_VK_TO_VSC);
	ToUnicode(nextMusicInput.vKey, 0, kb, c, 1, 0);
	
	std::wstring wname(5, 0);
	swprintf_s(&wname.front(), 5, L"(%c)", *c);
	nextMusicLabel.text(wname);

	// Stop Music
	GetKeyboardState(kb);
	MapVirtualKey(stopMusicInput.vKey, MAPVK_VK_TO_VSC);
	ToUnicode(stopMusicInput.vKey, 0, kb, c, 1, 0);

	wname = std::wstring(5, 0);
	swprintf_s(&wname.front(), 5, L"(%c)", *c);
	stopMusicLabel.text(wname);

	// Sounds
	for (Sound& sound : sounds) {
		GetKeyboardState(kb);
		MapVirtualKey(sound.vKey, MAPVK_VK_TO_VSC);
		ToUnicode(sound.vKey, 0, kb, c, 1, 0);
		
		wname = std::wstring(sound.name.size() + 20, 0);
		//wchar_t *wname = (wchar_t *) calloc(sound.name.size() + 6, sizeof(wchar_t));

		std::copy(sound.name.begin(), sound.name.end(), wname.begin());
		wchar_t* end_ptr = (&wname.front() + sound.name.size());
		wchar_t type = (sound.is_music ? L'M' : L'S');
		swprintf_s(end_ptr, 20, L" - %c - (%c)", type, *c);

		itemList.push_back(wname);
		//itemList.push_back(sound.name + L"     " + c);
	}
}

void SoundboardDisplay::updateVolumeLabel() {
	char text[5] = {0};
	sprintf_s(text, "%d%%", this->volumeSlider.value());

	this->volumeLabel.text(text);
}

void SoundboardDisplay::sendVolumeUpdate() {
	if (!app.isConnected())
		return;

	std::string uuid = boost::lexical_cast<std::string>(boost::uuids::random_generator()());

	app.sendData({
		{"type", "broadcast"},
		{"id", uuid},
		{"channel", "MusicVolume"},
		{"content", {
				{"volume", this->volumeSlider.value()},
			}
		}
		});
}