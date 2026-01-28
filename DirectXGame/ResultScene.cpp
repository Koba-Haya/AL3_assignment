#include "ResultScene.h"
#include "SoundManager.h"

#include <cmath>

namespace {
constexpr float kFadeTimeSec = 1.0f;

constexpr float kFixedDeltaSec = 1.0f / 60.0f;

// タイトル上下アニメ
constexpr float kFloatAmplitudePx = 12.0f;
constexpr float kFloatCycleSec = 2.0f;

// SPACE点滅（通常/高速）
constexpr float kBlinkCycleNormalSec = 0.8f;
constexpr float kBlinkCycleFastSec = 0.2f;
} // namespace

void ResultScene::Initialize() {
	finished_ = false;
	animTimeSec_ = 0.0f;

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, kFadeTimeSec_);
	phase_ = Phase::kFadeIn;

	texClear_ = TextureManager::Load("scene/clear.png");
	texFailed_ = TextureManager::Load("scene/failed.png");

	const Vector2 screen = {1280.0f, 720.0f};
	const Vector2 size = {1280.0f, 720.0f};
	basePos_ = {(screen.x - size.x) * 0.5f, (screen.y - size.y) * 0.5f};

	clearSprite_ = Sprite::Create(texClear_, basePos_);
	clearSprite_->SetSize(size);

	failedSprite_ = Sprite::Create(texFailed_, basePos_);
	failedSprite_->SetSize(size);

	backTextureHandle_ = TextureManager::Load("scene/back.png");
	backBasePos_ = {0.0f, 0.0f};
	backSprite_ = Sprite::Create(backTextureHandle_, backBasePos_);
	backSprite_->SetSize(Vector2(1280.0f, 720.0f));

	spaceTextureHandle_ = TextureManager::Load("scene/title_space.png");
	spaceBasePos_ = {0.0f, 0.0f};
	spaceSprite_ = Sprite::Create(spaceTextureHandle_, spaceBasePos_);
	spaceSprite_->SetSize(Vector2(1280.0f, 720.0f));

	SoundManager::Instance().PlayBgmResult(0.25f);
}

void ResultScene::Update() {
	animTimeSec_ += kFixedDeltaSec;
	blinkTimeSec_ += kFixedDeltaSec;

	const float phase = (2.0f * 3.14159265f) * (animTimeSec_ / kFloatCycleSec);
	const float offsetY = std::sinf(phase) * kFloatAmplitudePx;

	if (clearSprite_) {
		clearSprite_->SetPosition({basePos_.x, basePos_.y + offsetY});
	}
	if (failedSprite_) {
		failedSprite_->SetPosition({basePos_.x, basePos_.y + offsetY});
	}

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			fade_->Stop();
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain: {
		auto* in = Input::GetInstance();
		if (in->TriggerKey(DIK_SPACE) || in->PushKey(DIK_SPACE)) {
			SoundManager::Instance().PlaySeDecision(0.85f);
			blinkFast_ = true;
			fade_->Start(Fade::Status::FadeOut, kFadeTimeSec_);
			blackHoldFrames_ = 0;
			phase_ = Phase::kFadeOut;
		}
	} break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			if (blackHoldFrames_ < 2) {
				++blackHoldFrames_;
			} else {
				finished_ = true;
			}
		}
		break;
	}
}

void ResultScene::Draw() {
	Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());

	if (backSprite_) {
		backSprite_->Draw();
	}

	if (spaceSprite_) {
		const float cycle = blinkFast_ ? kBlinkCycleFastSec : kBlinkCycleNormalSec;
		const float t = std::fmod(blinkTimeSec_, cycle);
		const float alpha = (t < cycle * 0.5f) ? 1.0f : 0.0f;

		spaceSprite_->SetColor({1.0f, 1.0f, 1.0f, alpha});
		spaceSprite_->Draw();
	}

	if (kind_ == Kind::kClear) {
		if (clearSprite_) {
			clearSprite_->Draw();
		}
	} else {
		if (failedSprite_) {
			failedSprite_->Draw();
		}
	}

	Sprite::PostDraw();

	if (fade_) {
		fade_->Draw();
	}
}
