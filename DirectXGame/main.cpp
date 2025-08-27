#include "GameScene.h"
#include "TitleScene.h"
#include "ResultScene.h"
#include <KamataEngine.h>
#include <Windows.h>

using namespace KamataEngine;

GameScene* gameScene = nullptr;
TitleScene* titleScene = nullptr;
ResultScene* resultScene = nullptr;

enum class Scene { kUnknown = 0, kTitle, kGame, kResult };
Scene scene = Scene::kUnknown;

void ChangeScene();
void UpdateScene();
void DrawScene();

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"LE2B_10_コバヤシ_ハヤト_棘走");
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	scene = Scene::kTitle;
	titleScene = new TitleScene;
	titleScene->Initialize(false); // ★最初は透明で開始（戻りフェードインしない）

	while (true) {
		if (KamataEngine::Update()) {
			break;
		}

		dxCommon->PreDraw();
		KamataEngine::Model::PreDraw(dxCommon->GetCommandList());

		ChangeScene();
		UpdateScene();
		DrawScene();

		KamataEngine::Model::PostDraw();
		dxCommon->PostDraw();
	}
	delete titleScene;
	delete gameScene;
	delete resultScene;

	KamataEngine::Finalize();
	return 0;
}

void ChangeScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene && titleScene->IsFinished()) {
			scene = Scene::kGame;
			delete titleScene;
			titleScene = nullptr;
			gameScene = new GameScene;
			gameScene->Initialize();
		}
		break;

	case Scene::kGame:
		if (gameScene && gameScene->IsFinished()) {
			// ★GameSceneの結果でResultSceneへ
			auto res = gameScene->GetResult(); // ← 追加したゲッター
			delete gameScene;
			gameScene = nullptr;

			if (res == GameScene::Result::kClear) {
				resultScene = new ResultScene(ResultScene::Kind::kClear);
				resultScene->Initialize();
				scene = Scene::kResult;
			} else { // kFailed or その他
				resultScene = new ResultScene(ResultScene::Kind::kFailed);
				resultScene->Initialize();
				scene = Scene::kResult;
			}
		}
		break;

	case Scene::kResult:
		if (resultScene && resultScene->IsFinished()) {
			scene = Scene::kTitle;
			delete resultScene;
			resultScene = nullptr;

			titleScene = new TitleScene;
			titleScene->Initialize(true); // ★ゲームから戻ったのでフェードイン
		}
		break;

	default:
		break;
	}
}

void UpdateScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene)
			titleScene->Update();
		break;
	case Scene::kGame:
		if (gameScene)
			gameScene->Update();
		break;
	case Scene::kResult:
		if (resultScene)
			resultScene->Update();
		break;
	default:
		break;
	}
}

void DrawScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene)
			titleScene->Draw();
		break;
	case Scene::kGame:
		if (gameScene)
			gameScene->Draw();
		break;
	case Scene::kResult:
		if (resultScene)
			resultScene->Draw();
		break;
	default:
		break;
	}
}
