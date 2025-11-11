#define NOMINMAX
#include "Player.h"
#include "MapChipField.h"
#include <algorithm>
#include <assert.h>
#include <numbers>

using namespace KamataEngine;

void Player::Initialize(Model* model, Camera* camera, const Vector3& position) {
	// モデル・カメラの登録とワールド変換の初期化
	assert(model);
	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	// 初期の向き（+Xが右向き）
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
}

void Player::StartAttack() {
	// ======= 攻撃開始条件チェック =======
	// 死亡中は不可
	if (state_ == ActionState::Dead)
		return;
	// クールタイム中は不可
	if (attackCooldownLeft_ > 0.0f)
		return;
	// 既に攻撃中は不可
	if (state_ == ActionState::AttackWindup || state_ == ActionState::AttackActive || state_ == ActionState::AttackRecovery)
		return;

	// ======= 攻撃開始処理 =======
	// フェーズ遷移と各タイマーの初期化
	state_ = ActionState::AttackWindup;
	attackTimer_ = 0.0f;
	attackHitboxActive_ = false;

	// クールタイムのセット
	attackCooldownLeft_ = attackCooldownSec_;
}

void Player::Update() {
	// 固定デルタタイム
	float dt = 1.0f / 60.0f;

	// 攻撃クールタイムの更新
	if (attackCooldownLeft_ > 0.0f) {
		attackCooldownLeft_ = std::max(0.0f, attackCooldownLeft_ - dt);
	}

	// 行動ステート毎の更新処理
	switch (state_) {
	case ActionState::Move: {
		// 1) ジャンプ入力のバッファ（押した瞬間だけ記録）
		if (Input::GetInstance()->TriggerKey(DIK_UP)) {
			jumpBufferLeft_ = jumpBufferTime_;
		}

		// 2) 入力に基づく移動速度の更新（加減速/減衰、重力など）
		Move();

		// 3) マップ衝突判定の準備
		CollisionMapInfo collisionMapInfo{};
		collisionMapInfo.moveAmount_ = velocity_; // 速度ベクトルを「希望移動量」として渡す

		// 4) マップ衝突判定（上下左右の順で検出し、移動量をクランプ）
		MapCollisionDetection(collisionMapInfo);

		// 5) 判定結果に基づいて実際の位置を更新
		ApplyCollisionMove(collisionMapInfo);

		// 6) 衝突の反応（速度の補正や接地/壁フラグの更新）
		HandleCeilingCollision(collisionMapInfo); // 天井ヒット時は縦速度0・ジャンプバッファ潰し
		HandleGroundCollision(collisionMapInfo);  // 地面ヒット時は縦速度0・接地ON・ジャンプリセット
		HandleWallCollision(collisionMapInfo);    // 壁ヒット時は横速度0（条件付）・壁中のジャンプ抑止

		// 7) コヨーテタイマー（空中猶予時間）の更新
		if (onGround_) {
			coyoteLeft_ = coyoteTime_; // 接地中は常に満タン
		} else {
			coyoteLeft_ = std::max(0.0f, coyoteLeft_ - dt);
		}

		// 8) ジャンプ成立判定（バッファ × 接地/コヨーテ/空中ジャンプ、ただし壁接触中は不可）
		{
			bool wantJump = (jumpBufferLeft_ > 0.0f);                              // 入力されて猶予時間内か
			bool onWall = collisionMapInfo.onWallCollision_;                       // 壁に触れているか
			bool canGroundJump = (onGround_ || coyoteLeft_ > 0.0f);                // 接地またはコヨーテ内か
			bool canAirJump = (!onGround_ && !onWall && (jumpsUsed_ < maxJumps_)); // 空中回数が残っているか

			if (wantJump && !onWall && (canGroundJump || canAirJump)) {
				// ジャンプ確定：上向き初速付与、状態/カウンタ更新
				velocity_.y = kJumpAcceleration;
				onGround_ = false;
				++jumpsUsed_;           // 地上/コヨーテジャンプも回数に含める
				jumpBufferLeft_ = 0.0f; // バッファ消費
				coyoteLeft_ = 0.0f;     // コヨーテ消費
			}
		}

		// 9) 入力バッファの減衰
		jumpBufferLeft_ = std::max(0.0f, jumpBufferLeft_ - dt);

		// 10) 左右向きの旋回制御（一定時間でイーズ補間）
		if (turnTimer_ > 0.0f) {
			turnTimer_ -= 1.0f / 60.0f;
			float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
			float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
			float t = 1.0f - std::clamp(turnTimer_ / kTimeTurn, 0.0f, 1.0f);
			float easedT = EaseInOut(t);
			worldTransform_.rotation_.y = std::lerp(turnFirstRotationY_, destinationRotationY, easedT);
		}

		// 11) 攻撃入力（押した瞬間のみ）
		if ((Input::GetInstance()->TriggerKey(DIK_SPACE)) && attackCooldownLeft_ <= 0.0f) {
			StartAttack();
			break;
		}

		// 12) ワールド行列の更新とGPU転送
		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		break;
	}
	case ActionState::AttackWindup: {
		// 溜めフェーズ（Windup）：見た目幅を縮める、Y物理は継続
		attackTimer_ += dt;
		worldTransform_.scale_.z = std::lerp(atk_.widthMax, atk_.widthMin, std::clamp(attackTimer_ / atk_.windup, 0.0f, 1.0f));

		// 縦方向だけ衝突処理（このフェーズではX移動なし）
		CollisionMapInfo info{};
		info.moveAmount_ = {0.0f, velocity_.y, 0.0f};
		MapCollisionDetection(info);
		ApplyCollisionMove(info);
		HandleCeilingCollision(info);
		HandleWallCollision(info); // 横ヒットは速度のみ処理（位置は動かしていない）

		// 次フェーズ遷移
		if (attackTimer_ >= atk_.windup) {
			state_ = ActionState::AttackActive;
			attackTimer_ = 0.0f;
			attackHitboxActive_ = true;
		}
		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		break;
	}

	case ActionState::AttackActive: {
		// 発生フェーズ（Active）：当たり判定ON、見た目幅を戻す、前方へ突進（落下は止める）
		attackTimer_ += dt;

		// 見た目幅の補間（Zスケール）
		{
			float tA = std::clamp(attackTimer_ / atk_.active, 0.0f, 1.0f);
			worldTransform_.scale_.z = std::lerp(atk_.widthMin, atk_.widthMax, tA);
		}

		// X方向の突進（Yは固定）。Easeで速度変化させつつ、壁衝突は解決
		float tA = std::clamp(attackTimer_ / std::max(atk_.active, 1e-6f), 0.0f, 1.0f);
		float k = EaseOutCubic(tA);
		float perSec = atk_.lungeDistance / std::max(atk_.active, 1e-6f);
		float step = perSec * (0.7f + 0.6f * k) * dt;
		float dir = (lrDirection_ == LRDirection::kRight) ? +1.0f : -1.0f;

		CollisionMapInfo info{};
		info.moveAmount_ = {dir * step, 0.0f, 0.0f}; // 縦は動かさない
		MapCollisionDetection(info);
		ApplyCollisionMove(info);
		HandleCeilingCollision(info);
		HandleWallCollision(info);

		// 攻撃判定AABBの更新（見た目に合わせて伸縮）
		BuildAttackAABB();

		// 次フェーズ遷移
		if (attackTimer_ >= atk_.active) {
			state_ = ActionState::AttackRecovery;
			attackTimer_ = 0.0f;
			attackHitboxActive_ = false;
		}
		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		break;
	}
	case ActionState::AttackRecovery: {
		// 余韻フェーズ（Recovery）：幅を徐々に戻しつつ、通常の縦物理に復帰
		attackTimer_ += dt;

		// 見た目幅をふわっと戻す
		worldTransform_.scale_.z = std::lerp(worldTransform_.scale_.z, atk_.widthMax, 0.25f);

		// 縦物理（重力/落下速度制限）
		if (!onGround_) {
			velocity_.y -= kGravityAcceleration;
			velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
		}

		// 縦のみ移動解決（地面判定は復帰）
		CollisionMapInfo info{};
		info.moveAmount_ = {0.0f, velocity_.y, 0.0f};
		MapCollisionDetection(info);
		ApplyCollisionMove(info);
		HandleCeilingCollision(info);
		HandleGroundCollision(info);
		HandleWallCollision(info);

		// 移行：攻撃終了 → 通常移動
		if (attackTimer_ >= atk_.recovery) {
			state_ = ActionState::Move;
			attackTimer_ = 0.0f;
			worldTransform_.scale_.z = atk_.widthMax;
		}

		WorldTransformUpdate(worldTransform_);
		worldTransform_.TransferMatrix();
		break;
	}

	case ActionState::Dead:
		// 死亡中：必要ならノックバック等を記述（現状は未実装）
		break;
	}
}

void Player::UpdateFreeze() {
	// 凍結中：入力・物理・衝突・タイマーは進めない
	// 位置/回転/拡縮→ワールド行列生成→転送のみ行う
	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Player::Draw() {
	// 3Dモデル描画
	model_->Draw(worldTransform_, *camera_);
}

void Player::Move() {
	// 地上状態の左右入力処理（加速/減衰、向きの更新）
	if (onGround_) {
		if (Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_LEFT)) {
			Vector3 acceleration = {};

			if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
				// 逆方向慣性の軽減
				if (velocity_.x < 0.0f) {
					velocity_.x *= (1.0f - kAttenuation);
				}
				// 正方向へ加速
				acceleration.x += kAcceleration;

				// 右向きへの旋回トリガ
				if (lrDirection_ != LRDirection::kRight) {
					lrDirection_ = LRDirection::kRight;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTimeTurn;
				}
			} else if (Input::GetInstance()->PushKey(DIK_LEFT)) {
				// 逆方向慣性の軽減
				if (velocity_.x > 0.0f) {
					velocity_.x *= (1.0f - kAttenuation);
				}
				// 負方向へ加速
				acceleration.x -= kAcceleration;

				// 左向きへの旋回トリガ
				if (lrDirection_ != LRDirection::kLeft) {
					lrDirection_ = LRDirection::kLeft;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTimeTurn;
				}
			}

			// 速度更新と最大速度クランプ
			velocity_.x += acceleration.x;
			velocity_.x = std::clamp(velocity_.x, -kRimitRunSpeed, kRimitRunSpeed);
		} else {
			// 入力が無ければ減衰
			velocity_.x *= (1.0f - kAttenuation);
		}

		// ジャンプ処理は Update() の「バッファ/コヨーテ/空中ジャンプ」で一元管理
	}

	// 空中：重力の適用と落下速度のクランプ
	if (!onGround_) {
		velocity_.y -= kGravityAcceleration;
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}
}

const KamataEngine::WorldTransform& Player::GetWorldTransform() const { return worldTransform_; }

void Player::SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; };

void Player::MapCollisionDetection(CollisionMapInfo& info) {
	// 上下左右の順で衝突検出（移動量のクランプと各フラグ設定を行う）
	MapCollisionDetectionUp(info);
	MapCollisionDetectionDown(info);
	MapCollisionDetectionRight(info);
	MapCollisionDetectionLeft(info);
}

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	// 当たり判定矩形（幅×高さ）の4隅座標を、中心からのオフセットで取得
	static const Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0}, // 右下
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0}, // 左下
	    {+kWidth / 2.0f, +kHeight / 2.0f, 0}, // 右上
	    {-kWidth / 2.0f, +kHeight / 2.0f, 0}  // 左上
	};
	return {center.x + offsetTable[static_cast<uint32_t>(corner)].x, center.y + offsetTable[static_cast<uint32_t>(corner)].y, center.z + offsetTable[static_cast<uint32_t>(corner)].z};
}

void Player::MapCollisionDetectionUp(CollisionMapInfo& info) {
	// 上方向へ動くときだけ天井判定
	if (info.moveAmount_.y <= 0.0f) {
		return;
	}

	// 1) 希望移動後の位置で、上側2点（左上/右上）を調べる
	std::array<Vector3, 2> positionsNew;
	Vector3 movedPosition = {worldTransform_.translation_.x + info.moveAmount_.x, worldTransform_.translation_.y + info.moveAmount_.y, worldTransform_.translation_.z + info.moveAmount_.z};
	positionsNew[0] = CornerPosition(movedPosition, Corner::kLeftTop);
	positionsNew[1] = CornerPosition(movedPosition, Corner::kRightTop);

	// 2) 各点のタイルをチェック（Blockのみ衝突）
	bool hit = false;
	Rect hitRect = {0.0f, 0.0f, 0.0f, 0.0f};

	IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[0]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
	}
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[1]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
	}

	// 3) 衝突していて、かつセル境界をまたいだときだけ上方向の移動量をクランプ
	if (hit) {
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition({worldTransform_.translation_.x, worldTransform_.translation_.y + kHeight / 2.0f, worldTransform_.translation_.z});
		if (indexSetNow.yIndex != indexSet.yIndex) {
			float movedTop = movedPosition.y + kHeight / 2.0f;
			info.moveAmount_.y = std::max(0.0f, hitRect.bottom - (movedTop - info.moveAmount_.y));
			info.onCeilingCollision_ = true;
		}
	}
}

void Player::MapCollisionDetectionDown(CollisionMapInfo& info) {
	// 下方向へ動くときだけ床判定
	if (info.moveAmount_.y >= 0.0f) {
		return;
	}

	// 1) 希望移動後の位置で、下側2点（左下/右下）を調べる
	std::array<Vector3, 2> positionsNew;
	Vector3 movedPosition = {worldTransform_.translation_.x + info.moveAmount_.x, worldTransform_.translation_.y + info.moveAmount_.y, worldTransform_.translation_.z + info.moveAmount_.z};
	positionsNew[0] = CornerPosition(movedPosition, Corner::kLeftBottom);
	positionsNew[1] = CornerPosition(movedPosition, Corner::kRightBottom);

	// 2) 各点のタイルをチェック（Blockのみ衝突）
	bool hit = false;
	Rect hitRect = {};

	IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[0]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
	}
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[1]);
	if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock) {
		hit = true;
		hitRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
	}

	// 3) 衝突していて、かつセル境界をまたいだときだけ下方向の移動量をクランプ
	if (hit) {
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition({worldTransform_.translation_.x, worldTransform_.translation_.y - kHeight / 2.0f, worldTransform_.translation_.z});
		if (indexSetNow.yIndex != indexSet.yIndex) {
			float movedBottom = movedPosition.y - kHeight / 2.0f;
			info.moveAmount_.y = std::min(0.0f, hitRect.top - (movedBottom - info.moveAmount_.y));
			info.onGroundCollision_ = true;
		}
	}
}

void Player::MapCollisionDetectionRight(CollisionMapInfo& info) {
	// 右方向へ動くときだけ右壁判定
	if (info.moveAmount_.x <= 0.0f)
		return;

	const float eps = 0.001f;

	// A) 現在位置での重なり解消（デペネトレーション）
	{
		Vector3 cur = worldTransform_.translation_;
		Vector3 curRB = CornerPosition(cur, Corner::kRightBottom);
		Vector3 curRT = CornerPosition(cur, Corner::kRightTop);

		bool curHit = false;
		Rect curRect{};
		auto idx = mapChipField_->GetMapChipIndexSetByPosition(curRB);
		if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
			curRect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
			curHit = true;
		}
		idx = mapChipField_->GetMapChipIndexSetByPosition(curRT);
		if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
			curRect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
			curHit = true;
		}
		if (curHit) {
			// 現在の右端がタイル左端を越えていれば、左へ押し戻す量を算出
			float currentRight = cur.x + kWidth * 0.5f;
			float sep = curRect.left - currentRight - eps; // 通常は負（左へ押し戻し）
			if (sep > 0.0f) {
				// 希望移動量より大きい押し戻し量が必要なら更新
				info.moveAmount_.x = std::max(info.moveAmount_.x, sep);
				return;
			}
		}
	}

	// B) 希望移動後の位置での前進制限
	Vector3 moved = {worldTransform_.translation_.x + info.moveAmount_.x, worldTransform_.translation_.y + info.moveAmount_.y, worldTransform_.translation_.z + info.moveAmount_.z};
	Vector3 rightBottom = CornerPosition(moved, Corner::kRightBottom);
	Vector3 rightTop = CornerPosition(moved, Corner::kRightTop);

	bool hit = false;
	Rect rect{};
	auto idx = mapChipField_->GetMapChipIndexSetByPosition(rightBottom);
	if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
		hit = true;
		rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
	}
	idx = mapChipField_->GetMapChipIndexSetByPosition(rightTop);
	if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
		hit = true;
		rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
	}
	if (hit) {
		// 右端がタイル左端を越えないようにX移動量を0以上にクランプ
		float movedRight = moved.x + kWidth * 0.5f;
		info.moveAmount_.x = std::max(0.0f, rect.left - (movedRight - info.moveAmount_.x));
		info.clampedX_ = true;
		info.onWallCollision_ = true;
	}
}

void Player::MapCollisionDetectionLeft(CollisionMapInfo& info) {
	// 左方向へ動くときだけ左壁判定
	if (info.moveAmount_.x >= 0.0f)
		return;

	const float eps = 0.001f;

	// A) 現在位置での重なり解消（デペネトレーション）
	{
		Vector3 cur = worldTransform_.translation_;
		Vector3 curLB = CornerPosition(cur, Corner::kLeftBottom);
		Vector3 curLT = CornerPosition(cur, Corner::kLeftTop);

		// 斜め移動時に天井/床を“壁”と誤検出しないよう、縦方向に微小オフセット
		if (info.moveAmount_.y > 0.0f) { // 上昇中：上端を少し下げて天井巻き込みを回避
			curLT.y -= eps;
		} else if (info.moveAmount_.y < 0.0f) { // 下降中：下端を少し上げて床巻き込みを回避
			curLB.y += eps;
		}

		bool curHit = false;
		Rect curRect{};
		auto idx = mapChipField_->GetMapChipIndexSetByPosition(curLB);
		if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
			curRect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
			curHit = true;
		}
		idx = mapChipField_->GetMapChipIndexSetByPosition(curLT);
		if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
			curRect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
			curHit = true;
		}
		if (curHit) {
			// 現在の左端がタイル右端を越えていれば、右へ押し戻す量を算出
			float currentLeft = cur.x - kWidth * 0.5f;
			float sep = curRect.right - currentLeft + eps; // 通常は正（右へ押し戻し）
			if (sep > 0.0f) {
				info.moveAmount_.x = std::max(info.moveAmount_.x, sep);
				return;
			}
		}
	}

	// B) 希望移動後の位置での前進制限
	Vector3 moved = {worldTransform_.translation_.x + info.moveAmount_.x, worldTransform_.translation_.y + info.moveAmount_.y, worldTransform_.translation_.z + info.moveAmount_.z};
	Vector3 leftBottom = CornerPosition(moved, Corner::kLeftBottom);
	Vector3 leftTop = CornerPosition(moved, Corner::kLeftTop);

	// 斜め移動時の誤検出回避（天井/床を壁として拾わない微調整）
	if (info.moveAmount_.y > 0.0f) { // 上昇中
		leftTop.y -= eps;
	} else if (info.moveAmount_.y < 0.0f) { // 下降中
		leftBottom.y += eps;
	}

	bool hit = false;
	Rect rect{};
	auto idx = mapChipField_->GetMapChipIndexSetByPosition(leftBottom);
	if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
		hit = true;
		rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
	}
	idx = mapChipField_->GetMapChipIndexSetByPosition(leftTop);
	if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) == MapChipType::kBlock) {
		hit = true;
		rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
	}
	if (hit) {
		// 左端がタイル右端を越えないようにX移動量を0以下にクランプ
		float movedLeft = moved.x - kWidth * 0.5f;
		info.moveAmount_.x = std::min(0.0f, rect.right - (movedLeft - info.moveAmount_.x));
		info.onWallCollision_ = true;
		info.clampedX_ = true;
	}
}

void Player::ApplyCollisionMove(const CollisionMapInfo& info) {
	// クランプ済みの移動量を位置へ反映
	worldTransform_.translation_.x += info.moveAmount_.x;
	worldTransform_.translation_.y += info.moveAmount_.y;
	worldTransform_.translation_.z += info.moveAmount_.z;
}

void Player::HandleCeilingCollision(const CollisionMapInfo& info) {
	// 天井に当たったら縦速度を0にし、直後の誤ジャンプを抑止（バッファ破棄）
	if (info.onCeilingCollision_) {
		velocity_.y = 0.0f;
		jumpBufferLeft_ = 0.0f;
	}
}

void Player::HandleGroundCollision(const CollisionMapInfo& info) {
	// 地面に当たったら縦速度0・接地ON・ジャンプ回数リセット
	if (info.onGroundCollision_) {
		velocity_.y = 0.0f;
		onGround_ = true;
		jumpsUsed_ = 0;
	} else {
		// 接地していないフレームは空中扱い
		onGround_ = false;
	}
}

void Player::HandleWallCollision(const CollisionMapInfo& info) {
	// 壁衝突の反応
	if (info.onWallCollision_) {
		// Xが実際にクランプされた かつ 天井同時ヒットではない場合のみ横速度をゼロ
		if (info.clampedX_ && !info.onCeilingCollision_) {
			velocity_.x = 0.0f;
		} else {
			// 天井同時ヒット時は横慣性を残す（必要なら減衰させる）
			// velocity_.x *= 0.98f;
		}
		// 壁接触中はジャンプ多段入力を抑止（バッファ・コヨーテ破棄、ただし回数リセットはしない）
		jumpBufferLeft_ = 0.0f;
		coyoteLeft_ = 0.0f;
		onGround_ = false; // 壁は地面ではない
	}
}

Vector3 Player::GetWorldPosition() {
	// ワールド行列の平行移動成分（Tx,Ty,Tz）を抜き出して返す
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB Player::GetAABB() {
	// 現在位置とキャラサイズからAABBを生成（X/Zは幅、Yは高さ）
	Vector3 worldPos = GetWorldPosition();
	AABB aabb;
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};
	return aabb;
}

void Player::OnCollision(const Enemy* enemy) {
	// 敵とのゲーム側衝突反応（ここではデスと軽いノックアップ）
	(void)enemy;
	isDead_ = true;
	velocity_.y = kJumpAcceleration;
}

void Player::UpdateAttack(float dt) {
	// 攻撃の3フェーズ（Windup/Active/Recovery）を経過時間で制御
	attackTimer_ += dt;

	const float T_w = atk_.windup;
	const float T_a = atk_.active;
	const float T_r = atk_.recovery;
	const float T_all = T_w + T_a + T_r;

	// 通常スケール
	const Vector3 baseScale = {1.0f, 1.0f, 1.0f};

	// 1) Windup：当たりなし、横幅（xスケール）を絞る
	if (attackTimer_ < T_w) {
		float t = attackTimer_ / T_w;
		float sx = std::lerp(1.0f, atk_.widthMin / atk_.widthMax, t);
		worldTransform_.scale_.x = sx;
		worldTransform_.scale_.y = baseScale.y;
		worldTransform_.scale_.z = baseScale.z;
		attackHitboxActive_ = false;
	}
	// 2) Active：当たりあり、横幅を戻しつつ前進
	else if (attackTimer_ < T_w + T_a) {
		float tA = (attackTimer_ - T_w) / T_a;
		float k = EaseOutCubic(tA);

		float sx = std::lerp(atk_.widthMin / atk_.widthMax, 1.0f, k);
		worldTransform_.scale_.x = sx;
		worldTransform_.scale_.y = baseScale.y;
		worldTransform_.scale_.z = baseScale.z;

		// 向きに応じて前方向(+X)ベクトルを回転
		Vector3 forward = {1.0f, 0.0f, 0.0f};
		Matrix4x4 rotY = MakeRotateYMatrix(worldTransform_.rotation_.y);
		forward = TransformNormal(forward, rotY);

		// 少し前進
		float step = (atk_.lungeDistance * k * dt / std::max(T_a, 1e-6f));
		worldTransform_.translation_.x += forward.x * step;
		worldTransform_.translation_.y += forward.y * step;
		worldTransform_.translation_.z += forward.z * step;

		attackHitboxActive_ = true;
		BuildAttackAABB(); // 見た目に合わせてAABB更新
	}
	// 3) Recovery：当たりなし、スケールを通常へ戻す
	else {
		attackHitboxActive_ = false;
		worldTransform_.scale_ = baseScale;

		// 攻撃終了 → 通常移動へ戻す
		if (attackTimer_ >= T_all) {
			attackTimer_ = 0.0f;
			state_ = ActionState::Move;
		}
	}

	// 行列更新と転送
	WorldTransformUpdate(worldTransform_);
	worldTransform_.TransferMatrix();
}

void Player::BuildAttackAABB() {
	// Active中の見た目に同期して攻撃AABBを構築
	float tA = std::clamp(attackTimer_ / std::max(atk_.active, 1e-6f), 0.0f, 1.0f);

	// 長さ/幅の現在値
	float rangeNow = std::lerp(atk_.rangeMin, atk_.rangeMax, tA);
	float widthNow = std::lerp(atk_.widthMin, atk_.widthMax, tA);

	// プレイヤー中心と前方
	const Vector3 p = worldTransform_.translation_;
	float dir = (lrDirection_ == LRDirection::kRight) ? +1.0f : -1.0f;

	// 攻撃箱中心は自キャラ前方へ半分分オフセット
	float halfLen = rangeNow * 0.5f;
	Vector3 center = {p.x + dir * (halfLen + 0.5f * kWidth), p.y, p.z};

	// AABBの半径（X:長さ、Y:高さ、Z:幅）
	float hx = halfLen;
	float hy = atk_.height * 0.5f;
	float hz = widthNow * 0.5f;

	attackAabb_.min = {center.x - hx, center.y - hy, center.z - hz};
	attackAabb_.max = {center.x + hx, center.y + hy, center.z + hz};
}