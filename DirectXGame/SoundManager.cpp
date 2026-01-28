#include "SoundManager.h"
#include <Windows.h>
#include <cstdio>

namespace {

const char* kBgmTitlePath = "sounds/bgm/game.wav";
const char* kBgmGamePath = "sounds/bgm/game.wav";
const char* kBgmResultPath = "sounds/bgm/game.wav";

const char* kSeDecisionPath = "sounds/se/space.wav";
const char* kSeDeathPath = "sounds/se/death.wav";
const char* kSeWirePath = "sounds/se/wire.wav";
const char* kSeGoalPath = "sounds/se/goal.wav";

static void DebugOut(const char* fmt, uint32_t a = 0, uint32_t b = 0) {
	char buf[256]{};
	std::snprintf(buf, sizeof(buf), fmt, a, b);
	OutputDebugStringA(buf);
	OutputDebugStringA("\n");
}

} // namespace

void SoundManager::Initialize(const std::string& directoryPath) {
	audio_ = Audio::GetInstance();
	audio_->Initialize(directoryPath);
	LoadAll();
}

void SoundManager::LoadAll() {
	bgmTitle_ = audio_->LoadWave(kBgmTitlePath);
	bgmGame_ = audio_->LoadWave(kBgmGamePath);
	bgmResult_ = audio_->LoadWave(kBgmResultPath);

	seDecision_ = audio_->LoadWave(kSeDecisionPath);
	seDeath_ = audio_->LoadWave(kSeDeathPath);
	seWire_ = audio_->LoadWave(kSeWirePath);
	seGoal_ = audio_->LoadWave(kSeGoalPath);

	// 0でも失敗とは限らないので “表示だけ” して確認できるようにする
	DebugOut("[Sound] bgmTitle=%u bgmGame=%u", bgmTitle_, bgmGame_);
	DebugOut("[Sound] bgmResult=%u seDecision=%u", bgmResult_, seDecision_);
}

void SoundManager::StopBgm() {
	if (!hasBgmVoice_) {
		return;
	}
	audio_->StopWave(bgmVoice_);
	hasBgmVoice_ = false;
	bgmVoice_ = 0;
}

void SoundManager::PlayBgmTitle(float volume) {
	StopBgm();
	bgmVoice_ = audio_->PlayWave(bgmTitle_, true, volume);
	hasBgmVoice_ = true;

	DebugOut("[Sound] PlayBgmTitle voice=%u playing=%u", bgmVoice_, audio_->IsPlaying(bgmVoice_) ? 1u : 0u);
}
void SoundManager::PlayBgmGame(float volume) {
	StopBgm();
	bgmVoice_ = audio_->PlayWave(bgmGame_, true, volume);
	hasBgmVoice_ = true;

	DebugOut("[Sound] PlayBgmGame voice=%u playing=%u", bgmVoice_, audio_->IsPlaying(bgmVoice_) ? 1u : 0u);
}
void SoundManager::PlayBgmResult(float volume) {
	StopBgm();
	bgmVoice_ = audio_->PlayWave(bgmResult_, true, volume);
	hasBgmVoice_ = true;

	DebugOut("[Sound] PlayBgmResult voice=%u playing=%u", bgmVoice_, audio_->IsPlaying(bgmVoice_) ? 1u : 0u);
}

void SoundManager::PlaySeDecision(float volume) { audio_->PlayWave(seDecision_, false, volume); }
void SoundManager::PlaySePlayerDeath(float volume) { audio_->PlayWave(seDeath_, false, volume); }
void SoundManager::PlaySeWireMove(float volume) { audio_->PlayWave(seWire_, false, volume); }
void SoundManager::PlaySeGoal(float volume) { audio_->PlayWave(seGoal_, false, volume); }
