#include "ddi/DownloadInternal.h"
#include "ddi/Selection.h"

namespace maa::ios::ddi::detail
{
namespace
{
// Payload metadata is pinned to doronz88/DeveloperDiskImage commit
// 6eae353ae694bda1c421d4a3eee5459ae59c99a1, never a moving branch.
constexpr auto kSource =
    "https://raw.githubusercontent.com/doronz88/DeveloperDiskImage/"
    "6eae353ae694bda1c421d4a3eee5459ae59c99a1/";
} // namespace

const Catalog& personalized_catalog()
{
    static const Catalog catalog {
        "27A5228h",
        std::string(kSource) + "PersonalizedImages/Xcode_iOS_DDI_Personalized",
        {
            { "BuildManifest.plist", 801505, "8edd4a2f4f4ef1fbd7bfe49785d8badc673d1395d1d94d85b132ca8ab5ecaf54", { } },
            { "Image.dmg", 15'733'248, "05fd807da5e19f030fa4941f24800c965c6c77982ab572dd5d1ef778fb69f9ca", { } },
            { "Image.dmg.trustcache", 1895, "36af60889ff5a737874a26daeb8e1a0139ebfebec6ec2e4d8f6a3c1bf1dce35c", { } },
        },
        Kind::Personalized,
    };
    return catalog;
}

const Catalog* classic_catalog(const ProductVersion& version)
{
    // These legacy files have no published SHA256 catalog. Their sizes and Git
    // blob IDs come from the fixed commit tree. Verify SHA1 over the Git object:
    // "blob " + decimal byte count + NUL + the payload, rather than bare SHA1.
    static const std::vector<Catalog> catalogs {
        { "11.4",
          std::string(kSource) + "DeveloperDiskImages/11.4",
          {
              { "DeveloperDiskImage.dmg", 6'501'400, { }, "7e530f535e91b1b4ed0f38cd71d37a23e99dece9" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "e92bb7a57f7bc91b50a8e6cc4dc100e1cf1fb706" },
          },
          Kind::Classic },
        { "12.0",
          std::string(kSource) + "DeveloperDiskImages/12.0",
          {
              { "DeveloperDiskImage.dmg", 13'001'220, { }, "d35cc563b51b998a8aa706e110df0ca875725d85" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "005aa15b38a15823661624886a412ac0a24b96fa" },
          },
          Kind::Classic },
        { "12.1",
          std::string(kSource) + "DeveloperDiskImages/12.1",
          {
              { "DeveloperDiskImage.dmg", 13'028'500, { }, "466fe92b1137bd128c9f7bbc0fc19ae4c647ef06" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "e5467e85940044a9456568e6d7caafeec8968569" },
          },
          Kind::Classic },
        { "12.2",
          std::string(kSource) + "DeveloperDiskImages/12.2",
          {
              { "DeveloperDiskImage.dmg", 13'494'517, { }, "e9c4f020eb04d32f4145dc5bfd641adadd58ab67" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "e0f5d7ea7c58d783af847078b18b7cf6b1c40cb7" },
          },
          Kind::Classic },
        { "12.3",
          std::string(kSource) + "DeveloperDiskImages/12.3",
          {
              { "DeveloperDiskImage.dmg", 13'495'230, { }, "dcda553cabbda769db1b211230172e9d87161917" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "6212de1574a0e35af58b245c1f65484a6d933e65" },
          },
          Kind::Classic },
        { "12.4",
          std::string(kSource) + "DeveloperDiskImages/12.4",
          {
              { "DeveloperDiskImage.dmg", 13'495'279, { }, "0fb8af3fd992879c258def48acc149343c716eff" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "7e5d2a52a4f9df911326ec66d6dd4640192f6d90" },
          },
          Kind::Classic },
        { "13.0",
          std::string(kSource) + "DeveloperDiskImages/13.0",
          {
              { "DeveloperDiskImage.dmg", 12'765'520, { }, "704e5c70ad1117d3d06476728115d50e4278dc64" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "59ab8572fd35f490b6c4cf32921a0538eae0c16f" },
          },
          Kind::Classic },
        { "13.1",
          std::string(kSource) + "DeveloperDiskImages/13.1",
          {
              { "DeveloperDiskImage.dmg", 12'827'529, { }, "ebf2dab7fbc4b1a32aa744d03e862d4db5db9dc3" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "5aebc4c1b10bd744210e5c175095ceb349e0c7dc" },
          },
          Kind::Classic },
        { "13.2",
          std::string(kSource) + "DeveloperDiskImages/13.2",
          {
              { "DeveloperDiskImage.dmg", 12'860'989, { }, "cde21b998a7a4f057fc9b7880d204c09d0ec62a1" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "12a0b4cce55b27e72a8ad3a6b434561a831c3e38" },
          },
          Kind::Classic },
        { "13.3",
          std::string(kSource) + "DeveloperDiskImages/13.3",
          {
              { "DeveloperDiskImage.dmg", 12'864'110, { }, "74a86a54351d3e1cd755ab906d5386ecff8bc9d8" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "02df2b85d26fcddcaca459b25cf12c843240ea19" },
          },
          Kind::Classic },
        { "13.4",
          std::string(kSource) + "DeveloperDiskImages/13.4",
          {
              { "DeveloperDiskImage.dmg", 13'826'482, { }, "0701424aaae1e11b25cdafe9fbbd34588d6764fa" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "89293c98ce55f0103f5ac06aeadcacfa79dbd20f" },
          },
          Kind::Classic },
        { "13.5",
          std::string(kSource) + "DeveloperDiskImages/13.5",
          {
              { "DeveloperDiskImage.dmg", 13'843'921, { }, "5f1cd5fc9d1b18c129fa1615b7e520eb7f34adf2" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "e647cec808a6a8a2b520fed8e1067f2bdc93221d" },
          },
          Kind::Classic },
        { "13.6",
          std::string(kSource) + "DeveloperDiskImages/13.6",
          {
              { "DeveloperDiskImage.dmg", 13'843'726, { }, "57b973aa4e8f1ee0d0fce53f87dbca39c8bc1eb4" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "b6eb5c23ef52bb18f2994ca84c43023b478e5926" },
          },
          Kind::Classic },
        { "13.7",
          std::string(kSource) + "DeveloperDiskImages/13.7",
          {
              { "DeveloperDiskImage.dmg", 13'882'301, { }, "c31ed3753facfbc4aae04bd187ab8afcd0baf28d" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "c528e1199429caf12bbd3a761dc5218f030e7b75" },
          },
          Kind::Classic },
        { "14.0",
          std::string(kSource) + "DeveloperDiskImages/14.0",
          {
              { "DeveloperDiskImage.dmg", 19'748'558, { }, "1633366d414375157bb78e938a21c49c59f053c6" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "b66a179249aba5152674328b022fe0a7822ffde6" },
          },
          Kind::Classic },
        { "14.1",
          std::string(kSource) + "DeveloperDiskImages/14.1",
          {
              { "DeveloperDiskImage.dmg", 19'776'817, { }, "fcf92720d5c11a8c8bd4483264c10b65b6e33029" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "5ae250b3dc8fbd116e4b2bf99a6711a9b7e04298" },
          },
          Kind::Classic },
        { "14.2",
          std::string(kSource) + "DeveloperDiskImages/14.2",
          {
              { "DeveloperDiskImage.dmg", 19'789'186, { }, "47bd02b1d3a570e03a59d109374d97c0a47d742e" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "b14af21ac4922486870c3672fed5f307bc4a0a32" },
          },
          Kind::Classic },
        { "14.3",
          std::string(kSource) + "DeveloperDiskImages/14.3",
          {
              { "DeveloperDiskImage.dmg", 19'876'664, { }, "b04355486ef718bd723a4c8650f78082cc62be2f" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "e04a8215df94295b4c019fe96f6b9fdd2df70c4f" },
          },
          Kind::Classic },
        { "14.4",
          std::string(kSource) + "DeveloperDiskImages/14.4",
          {
              { "DeveloperDiskImage.dmg", 19'878'885, { }, "c5131e03cebd87a5e7a2b65d561c859b97f512ab" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "f216a586908469abc8bbabe8ccfd38ba78777b5a" },
          },
          Kind::Classic },
        { "14.5",
          std::string(kSource) + "DeveloperDiskImages/14.5",
          {
              { "DeveloperDiskImage.dmg", 20'100'131, { }, "1b6767b68089d8b70c1d3c71d513c585bbd4e7d0" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "b6e8bd4e8d66dd4251da1619574fe1249d2fe822" },
          },
          Kind::Classic },
        { "14.6",
          std::string(kSource) + "DeveloperDiskImages/14.6",
          {
              { "DeveloperDiskImage.dmg", 20'081'202, { }, "86bdc58ff11563b0568c5d11ff769fc20168cbc5" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "77bc4f1a80067ba48010c13281c865d2a1d934b7" },
          },
          Kind::Classic },
        { "14.7",
          std::string(kSource) + "DeveloperDiskImages/14.7",
          {
              { "DeveloperDiskImage.dmg", 20'081'202, { }, "86bdc58ff11563b0568c5d11ff769fc20168cbc5" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "77bc4f1a80067ba48010c13281c865d2a1d934b7" },
          },
          Kind::Classic },
        { "14.8",
          std::string(kSource) + "DeveloperDiskImages/14.8",
          {
              { "DeveloperDiskImage.dmg", 20'081'202, { }, "86bdc58ff11563b0568c5d11ff769fc20168cbc5" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "77bc4f1a80067ba48010c13281c865d2a1d934b7" },
          },
          Kind::Classic },
        { "15.0",
          std::string(kSource) + "DeveloperDiskImages/15.0",
          {
              { "DeveloperDiskImage.dmg", 11'881'636, { }, "c1faa62fd75238d03ef96c0c26e8cf1a924c37fe" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "f97ee9d7785e6caf2a35efc54414c6bdf5160fa4" },
          },
          Kind::Classic },
        { "15.1",
          std::string(kSource) + "DeveloperDiskImages/15.1",
          {
              { "DeveloperDiskImage.dmg", 11'881'636, { }, "c1faa62fd75238d03ef96c0c26e8cf1a924c37fe" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "f97ee9d7785e6caf2a35efc54414c6bdf5160fa4" },
          },
          Kind::Classic },
        { "15.2",
          std::string(kSource) + "DeveloperDiskImages/15.2",
          {
              { "DeveloperDiskImage.dmg", 11'968'751, { }, "8769f6831f6d053be9a8c669270c608c65371b45" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "73de769f85844aea2eb5e8fb7270e6a4c9125d64" },
          },
          Kind::Classic },
        { "15.3",
          std::string(kSource) + "DeveloperDiskImages/15.3",
          {
              { "DeveloperDiskImage.dmg", 11'968'751, { }, "8769f6831f6d053be9a8c669270c608c65371b45" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "73de769f85844aea2eb5e8fb7270e6a4c9125d64" },
          },
          Kind::Classic },
        { "15.4",
          std::string(kSource) + "DeveloperDiskImages/15.4",
          {
              { "DeveloperDiskImage.dmg", 9'095'712, { }, "bff34b4853346bae3cee7056ba7b7b302fbee7a2" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "c19156b0d983955a8d87b91b66a5a430a4289a66" },
          },
          Kind::Classic },
        { "15.5",
          std::string(kSource) + "DeveloperDiskImages/15.5",
          {
              { "DeveloperDiskImage.dmg", 9'099'610, { }, "dff04de806e18f7d2b8e64c86b8a170652695780" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "3d7a9d70ceb8c4bdc6baf6aa478c434eb93c5b2c" },
          },
          Kind::Classic },
        { "15.6",
          std::string(kSource) + "DeveloperDiskImages/15.6",
          {
              { "DeveloperDiskImage.dmg", 9'101'847, { }, "5da69478bac9bb06e8abf7344afa1f0cf72a3a9d" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "7ff3fab78ff91314c1ba9092941c3c07f0966dec" },
          },
          Kind::Classic },
        { "15.7",
          std::string(kSource) + "DeveloperDiskImages/15.7",
          {
              { "DeveloperDiskImage.dmg", 9'064'832, { }, "f1d57e5a610c6647d3b70b8e69d5abf2bf6bef0a" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "9c5a3acb82029cf856da7ae6e78cb8e947f3fdb7" },
          },
          Kind::Classic },
        { "15.8",
          std::string(kSource) + "DeveloperDiskImages/15.8",
          {
              { "DeveloperDiskImage.dmg", 9'099'610, { }, "dff04de806e18f7d2b8e64c86b8a170652695780" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "3d7a9d70ceb8c4bdc6baf6aa478c434eb93c5b2c" },
          },
          Kind::Classic },
        { "16.0",
          std::string(kSource) + "DeveloperDiskImages/16.0",
          {
              { "DeveloperDiskImage.dmg", 8'464'365, { }, "22c520c0e3a00787864e56f26684f7a12cbbcc73" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "c76344989549ea69a5598918b6d2e6c93b5ec78e" },
          },
          Kind::Classic },
        { "16.1",
          std::string(kSource) + "DeveloperDiskImages/16.1",
          {
              { "DeveloperDiskImage.dmg", 8'448'885, { }, "865e19d43e632a27c786e003642f5fdcf0df4a3b" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "df51dbe79bbb96a1bc7015863d5649360614be9c" },
          },
          Kind::Classic },
        { "16.2",
          std::string(kSource) + "DeveloperDiskImages/16.2",
          {
              { "DeveloperDiskImage.dmg", 8'466'283, { }, "1a266248eb9764d61284f36d93eac2714b9b7af8" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "5b27ae043a2a1ab109dd98719b63ae075ff42369" },
          },
          Kind::Classic },
        { "16.3",
          std::string(kSource) + "DeveloperDiskImages/16.3",
          {
              { "DeveloperDiskImage.dmg", 8'466'283, { }, "1a266248eb9764d61284f36d93eac2714b9b7af8" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "5b27ae043a2a1ab109dd98719b63ae075ff42369" },
          },
          Kind::Classic },
        { "16.4",
          std::string(kSource) + "DeveloperDiskImages/16.4",
          {
              { "DeveloperDiskImage.dmg", 7'432'520, { }, "35f55979cfe16c265ba01c5981afa2d5cd2764c0" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "6705a5aac7bdb0b116e24328cc68f1b352fe70fe" },
          },
          Kind::Classic },
        { "16.5",
          std::string(kSource) + "DeveloperDiskImages/16.5",
          {
              { "DeveloperDiskImage.dmg", 7'427'565, { }, "240921c39817f49fa335428a358f1a4ab9a6df9d" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "153b51e82e815dcf906a3bb14c781a7160d412fa" },
          },
          Kind::Classic },
        { "16.6",
          std::string(kSource) + "DeveloperDiskImages/16.6",
          {
              { "DeveloperDiskImage.dmg", 7'432'520, { }, "35f55979cfe16c265ba01c5981afa2d5cd2764c0" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "6705a5aac7bdb0b116e24328cc68f1b352fe70fe" },
          },
          Kind::Classic },
        { "16.7",
          std::string(kSource) + "DeveloperDiskImages/16.7",
          {
              { "DeveloperDiskImage.dmg", 7'432'520, { }, "35f55979cfe16c265ba01c5981afa2d5cd2764c0" },
              { "DeveloperDiskImage.dmg.signature", 128, { }, "6705a5aac7bdb0b116e24328cc68f1b352fe70fe" },
          },
          Kind::Classic },
    };
    const auto wanted = std::to_string(version.major) + "." + std::to_string(version.minor);
    for (const auto& catalog : catalogs) {
        if (catalog.build == wanted) {
            return &catalog;
        }
    }
    return nullptr;
}

} // namespace maa::ios::ddi::detail
