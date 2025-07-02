#include "GameScene.h"

using namespace KamataEngine;

GameScene::~GameScene() {
	// 3Dモデルデータの開放
	delete model_;
}

void GameScene::Initialize() {
	// ファイル名を指定してテクスチャを読み込む
	textureHandle_ = TextureManager::Load("uvChecker.png");

	// 3Dモデルデータの生成
	model_ = Model::Create();
}

void GameScene::Update() {}

void GameScene::Draw() {}
