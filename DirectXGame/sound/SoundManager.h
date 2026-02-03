#pragma once
#include "KamataEngine.h"
#include <cstdint>
#include <string>

using namespace KamataEngine;

class SoundManager {
public:
	static SoundManager& Instance() {
		static SoundManager instance;
		return instance;
	}

	void Initialize(const std::string& directoryPath = "Resources/");
	void LoadAll();

	void PlayBgmTitle(float volume = 0.25f);
	void PlayBgmGame(float volume = 0.25f);
	void PlayBgmResult(float volume = 0.25f);
	void StopBgm();

	void PlaySeDecision(float volume = 0.8f);
	void PlaySePlayerDeath(float volume = 0.9f);
	void PlaySeWireMove(float volume = 0.7f);
	void PlaySeGoal(float volume = 0.9f);

private:
	SoundManager() = default;
	~SoundManager() = default;
	SoundManager(const SoundManager&) = delete;
	SoundManager& operator=(const SoundManager&) = delete;

	Audio* audio_ = nullptr;

	uint32_t bgmTitle_ = 0;
	uint32_t bgmGame_ = 0;
	uint32_t bgmResult_ = 0;

	uint32_t seDecision_ = 0;
	uint32_t seDeath_ = 0;
	uint32_t seWire_ = 0;
	uint32_t seGoal_ = 0;

	// 0 も有効になり得るので、別フラグで管理する
	bool hasBgmVoice_ = false;
	uint32_t bgmVoice_ = 0;
};
