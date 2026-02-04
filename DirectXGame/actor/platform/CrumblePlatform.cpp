#define NOMINMAX

#include "CrumblePlatform.h"
#include <algorithm>

void CrumblePlatform::Initialize(Model* model, Camera* camera, const Vector3& pos, int reviveSec, int fallSec, MapChipField* mapChipField) {
	model_ = model;
	camera_ = camera;

	mapChipField_ = mapChipField;
	if (mapChipField_) {
		tileIndex_ = mapChipField_->GetMapChipIndexSetByPosition(pos);
	}

	homePos_ = pos;

	worldTransform_.Initialize();
	worldTransform_.translation_ = pos;

	reviveSec = std::clamp(reviveSec, 1, 9);
	fallSec = std::clamp(fallSec, 1, 9);

	reviveTimeSec_ = static_cast<float>(reviveSec);
	fallTimeSec_ = static_cast<float>(fallSec);

	active_ = true;
	state_ = State::Stable;
	fallLeft_ = 0.0f;
	reviveLeft_ = 0.0f;

	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
	RebuildAABB_();
}

void CrumblePlatform::OnStepped() {
	if (!active_) {
		return;
	}

	// すでにカウント/落下中なら何もしない
	if (state_ == State::Counting || state_ == State::Falling) {
		return;
	}

	state_ = State::Counting;
	fallLeft_ = fallTimeSec_;
}

void CrumblePlatform::Update() {
	switch (state_) {
	case State::Stable:
		break;

	case State::Counting:
		fallLeft_ = std::max(0.0f, fallLeft_ - kFixedDt);
		if (fallLeft_ <= 0.0f) {
			// 当たりを消してから落下開始（プレイヤー/敵が即落ちる）
			if (mapChipField_) {
				mapChipField_->SetMapChipTypeByIndex(tileIndex_.xIndex, tileIndex_.yIndex, MapChipType::kBlank);
				mapChipField_->SetMapChipParamByIndex(tileIndex_.xIndex, tileIndex_.yIndex, 0);
			}
			state_ = State::Falling;
		}
		break;

	case State::Falling:
		worldTransform_.translation_.y -= fallSpeed_;

		// 画面外（十分下）に行ったら消滅→復活待ちへ
		if (worldTransform_.translation_.y <= vanishY_) {
			active_ = false;
			state_ = State::WaitingRevive;
			reviveLeft_ = reviveTimeSec_;
		}
		break;

	case State::WaitingRevive:
		reviveLeft_ = std::max(0.0f, reviveLeft_ - kFixedDt);
		if (reviveLeft_ <= 0.0f) {
			// 元位置へ戻す
			worldTransform_.translation_ = homePos_;

			// 当たり（タイル）を復活
			if (mapChipField_) {
				mapChipField_->SetMapChipTypeByIndex(tileIndex_.xIndex, tileIndex_.yIndex, MapChipType::kCrumbleFloor);
			}

			active_ = true;
			state_ = State::Stable;
			fallLeft_ = 0.0f;
		}
		break;
	}

	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
	RebuildAABB_();
}

void CrumblePlatform::Draw() const {
	if (!active_) {
		return;
	}
	if (!model_ || !camera_) {
		return;
	}
	model_->Draw(worldTransform_, *camera_);
}

void CrumblePlatform::RebuildAABB_() {
	// 落下中も AABB は更新（ただしタイル側の当たりは既に消している）
	const Vector3& p = worldTransform_.translation_;
	const float half = 0.5f;
	aabb_.min = {p.x - half, p.y - half, p.z - half};
	aabb_.max = {p.x + half, p.y + half, p.z + half};
}