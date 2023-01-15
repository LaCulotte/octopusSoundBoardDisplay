#pragma once
#include <boost/ui.hpp>
#include <octopus-connection/json.hpp>
#include <octopus-app/app.hpp>

#include <vector>

struct Sound {
	uint8_t vKey;
	std::string name;
	bool is_music;
};

class SoundboardDisplay {
public:
	SoundboardDisplay();

	bool processInput(uint8_t vKey, void* device, bool keyPressed);
	void show_modal() { window.show_modal(); }

private:
	OctopusApp app;
	std::vector<Sound> sounds;

	void addSound(uint8_t vKey, void* device, bool keyPressed);

	void connectToApp();
	void sendSound(uint8_t vKey, void* device, bool keyPressed);

	void updateItemList();
	void updateDisplay();

	void updateVolumeLabel();
	void sendVolumeUpdate();

	void addItemCb();
	void removeItemCb();

	Sound nextMusicInput = {0, "", false};
	void addNextMusicInput(uint8_t vKey, void* device, bool keyPressed);
	void sendNextMusic(bool keyPressed);
	void addNextMusicInputCb();

	Sound stopMusicInput = { 0, "", false };
	void addStopMusicInput(uint8_t vKey, void* device, bool keyPressed);
	void sendStopMusic(bool keyPressed);
	void addStopMusicInputCb();

	void addItem() {};

	void load();
	void save();

	bool addingInput = false;
	bool addingNextMusicInput = false;
	bool addingStopMusicInput = false;
	
	void* device = nullptr;

	boost::ui::dialog window;

	boost::ui::string_box connectAppInput;
	boost::ui::button connectAppButton;
	boost::ui::label connectAppStatus;


	boost::ui::string_box nameInput;
	boost::ui::button addItemButton;
	boost::ui::label addItemStatus;
	boost::ui::check_box isMusicBox;

	boost::ui::button nextMusicButton;
	boost::ui::label nextMusicLabel;
	boost::ui::button stopMusicButton;
	boost::ui::label stopMusicLabel;

	boost::ui::button updateVolumeButton;
	boost::ui::hslider volumeSlider;
	boost::ui::label volumeLabel;

	boost::ui::button deleteItemButton;

	boost::ui::list_box itemList;
};