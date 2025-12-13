plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.maa.framework.nativectrl"
    compileSdk = 34

    defaultConfig {
        minSdk = 24
        
        consumerProguardFiles("consumer-rules.pro")
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    sourceSets {
        getByName("main") {
            java.srcDirs("com")
            kotlin.srcDirs("com")
            manifest.srcFile("AndroidManifest.xml")
            res.srcDirs("res")
        }
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.12.0")
}
