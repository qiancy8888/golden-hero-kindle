#include "rpg.h"

namespace golden {
namespace {
using K = Skill;
using S = Sect;

constexpr SkillDef skills[] = {
    {u8"基本拳脚", S::None, K::Fist},
    {u8"基本剑术", S::None, K::Sword},
    {u8"基本轻功", S::None, K::Dodge},
    {u8"基本内功", S::None, K::Force},
    {"", S::None, K::Legacy},
    {u8"基本刀法", S::None, K::Saber},
    {u8"基本法术", S::None, K::Spell},
    {u8"基本杖法", S::None, K::Staff},
    {u8"基本鞭法", S::None, K::Whip},
    {u8"基本招架", S::None, K::Parry},
    {u8"读书识字", S::None, K::Literacy},
    {u8"驻颜术", S::Flower, K::Literacy},
    {u8"八卦刀", S::Bagua, K::Saber},
    {u8"八卦游身掌", S::Bagua, K::Fist},
    {u8"八阵八卦掌", S::Bagua, K::Fist},
    {u8"混元一气功", S::Bagua, K::Force},
    {u8"游龙身法", S::Bagua, K::Dodge},
    {u8"飞蝶身法", S::Flower, K::Dodge},
    {u8"花团鞭法", S::Flower, K::Whip},
    {u8"柳叶刀法", S::Flower, K::Saber},
    {u8"一剪梅花手", S::Flower, K::Fist},
    {u8"三花聚顶", S::Flower, K::Force},
    {u8"鹤翔身法", S::Lotus, K::Dodge},
    {u8"红莲教义", S::Lotus, K::Literacy},
    {u8"乱披风杖法", S::Lotus, K::Staff},
    {u8"太祖长拳", S::Lotus, K::Fist},
    {u8"普天同济", S::Lotus, K::Force},
    {u8"扶桑忍术", S::Iga, K::Literacy},
    {u8"无法拳", S::Iga, K::Fist},
    {u8"无影遁形", S::Iga, K::Dodge},
    {u8"川枫一刀流", S::Iga, K::Saber},
    {u8"太极剑", S::Taiji, K::Sword},
    {u8"太极拳", S::Taiji, K::Fist},
    {u8"太极神功", S::Taiji, K::Force},
    {u8"万流归一", S::Taiji, K::Parry},
    {u8"玄虚刀法", S::Taiji, K::Saber},
    {u8"踏雪无痕", S::Snow, K::Dodge},
    {u8"雪山内功", S::Snow, K::Force},
    {u8"入门十三式", S::Snow, K::Sword},
    {u8"雪山剑法", S::Snow, K::Sword},
    {u8"雪影擒拿手", S::Snow, K::Fist},
    {u8"五禽戏", S::Beast, K::Dodge},
    {u8"鳄灵刀", S::Beast, K::Saber},
    {u8"赤尻连拳", S::Beast, K::Fist},
    {u8"猛虎拳", S::Beast, K::Fist},
    {u8"鹰爪功", S::Beast, K::Fist},
    {u8"龙象般若功", S::Beast, K::Force},
    {u8"灵符身法", S::Maoshan, K::Dodge},
    {u8"天师掌法", S::Maoshan, K::Fist},
    {u8"谷衣心法", S::Maoshan, K::Force},
    {u8"天师正道", S::Maoshan, K::Spell},
    {u8"万鸦咒", S::Maoshan, K::Spell},
    {u8"玄冰咒", S::Maoshan, K::Spell},
    {u8"五雷咒", S::Maoshan, K::Spell},
    {u8"灵通心诀", S::Maoshan, K::Literacy},
};
static_assert(sizeof(skills) / sizeof(skills[0]) == skill_count, "One definition per skill");
}

const SkillDef& skill_def(Skill skill) { return skills[static_cast<std::size_t>(skill)]; }
const char* skill_name(Skill skill) { return skill_def(skill).name; }
}
