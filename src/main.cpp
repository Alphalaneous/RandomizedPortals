#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <random>

using namespace geode::prelude;

static constexpr std::array<int, 5> s_speedPortals = {
    200, 201, 202, 203, 1334
};

static int random(int min, int max) {
    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

class $modify(PlayLayer) {

    void showNewBest(bool p0, int p1, int p2, bool p3, bool p4, bool p5) {
        if (Mod::get()->getSettingValue<bool>("soft-toggle") && Mod::get()->getSettingValue<bool>("safe-mode")) return;
        PlayLayer::showNewBest(p0, p1, p2, p3, p4, p5);
    }

    void levelComplete() {
        if (Mod::get()->getSettingValue<bool>("soft-toggle") && Mod::get()->getSettingValue<bool>("safe-mode")) {
            bool testMode = m_isTestMode;
            m_isTestMode = true;
            PlayLayer::levelComplete();
            m_isTestMode = testMode;
            return;
        }
        PlayLayer::levelComplete();
    }

};

class $modify(GJGameLevel) {
    void savePercentage(int p0, bool p1, int p2, int p3, bool p4) {
        if (Mod::get()->getSettingValue<bool>("soft-toggle") && Mod::get()->getSettingValue<bool>("safe-mode")) return;
        GJGameLevel::savePercentage(p0, p1, p2, p3, p4);
    }
};

class $modify(MyGJBaseGameLayer, GJBaseGameLayer) {

    void setupLevelStart(LevelSettingsObject* p0) {
        if (!Mod::get()->getSettingValue<bool>("soft-toggle")) return GJBaseGameLayer::setupLevelStart(p0);

        // only seems to work for particles that aren't preloaded
        for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
            if (std::ranges::find(s_speedPortals, obj->m_objectID) != s_speedPortals.end()) {
                obj->m_objectID = s_speedPortals[random(0, s_speedPortals.size() - 1)];
                std::string frame = ObjectToolbox::sharedState()->intKeyToFrame(obj->m_objectID);
                std::string particleFrameName = utils::string::replace(frame, "_001.png", "");
                std::string particleString = fmt::format("{}{}_effect.plist-22", obj->m_objectID, particleFrameName);
                obj->m_particleString = particleString;
            }
        }

        GJBaseGameLayer::setupLevelStart(p0);

        int speed = random(0, 4);
        float playerSpeed = 0.9f;
        switch (speed) {
            case 1: {
                playerSpeed = 0.70f;
                break;
            }
            case 0: {
                playerSpeed = 0.9f;
                break;
            }
            case 2: {
                playerSpeed = 1.1f;
                break;
            }
            case 3: {
                playerSpeed = 1.3f;
                break;
            }
            case 4: {
                playerSpeed = 1.6f;
                break;
            }
        }

        m_player1->m_playerSpeed = playerSpeed;
        if (m_player2) m_player2->m_playerSpeed = playerSpeed;

        // somewhat evil hack
        queueInMainThread([self = Ref(this)] {
            queueInMainThread([self] {
                for (auto obj : CCArrayExt<GameObject*>(self->m_objects)) {
                    if (std::ranges::find(s_speedPortals, obj->m_objectID) != s_speedPortals.end()) {
                        std::string frame = ObjectToolbox::sharedState()->intKeyToFrame(obj->m_objectID);
                        CCSprite* spr = CCSprite::createWithSpriteFrameName(frame.c_str());
                        obj->setTextureRect(spr->getTextureRect(), spr->isTextureRectRotated(), spr->getTextureRect().size);
                        if (obj->m_glowSprite) {
                            std::string glowFrame = utils::string::replace(frame, "_001.png","_glow_001.png");
                            CCSprite* glowSpr = CCSprite::createWithSpriteFrameName(glowFrame.c_str());
                            obj->m_glowSprite->setTextureRect(glowSpr->getTextureRect(), glowSpr->isTextureRectRotated(), glowSpr->getTextureRect().size);
                        }
                        obj->updateOrientedBox();
                    }
                }
            });
        });
    }
};