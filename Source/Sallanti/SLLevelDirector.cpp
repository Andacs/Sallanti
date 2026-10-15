#include "SLLevelDirector.h"

#include "SLBuildComponent.h"
#include "SLAmbience.h"
#include "SLBus.h"
#include "SLFamily.h"
#include "SLGameMode.h"
#include "SLPlaceable.h"
#include "SLPlatform.h"
#include "SLPlayerController.h"
#include "Algo/Reverse.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogSLLevel, Log, All);

namespace
{
	FSLTask MakeTask(const TCHAR* Text, const TCHAR* Speech, const TCHAR* Done, const TCHAR* Hint, ESLTaskKind Kind, const TCHAR* Type,
		std::initializer_list<FName> Unlocks, int32 Count = 1, int32 Speaker = 0)
	{
		FSLTask T;
		T.Text = Text;
		T.Speech = Speech;
		T.DoneSpeech = Done;
		T.Hint = Hint;
		T.Kind = Kind;
		T.TypeId = Type;
		T.Unlocks = Unlocks;
		T.Count = Count;
		T.Speaker = Speaker;
		return T;
	}

	FSLNeed MakeNeed(ESLNeedKind Kind, const TCHAR* Type, const TCHAR* Label, const TCHAR* Complaint, int32 MaxCells = 4,
		int32 MinCells = 4, int32 FamilyIndex = 0)
	{
		FSLNeed N;
		N.Kind = Kind;
		N.Type = Type;
		N.Label = Label;
		N.Complaint = Complaint;
		N.MaxCells = MaxCells;
		N.MinCells = MinCells;
		N.FamilyIndex = FamilyIndex;
		return N;
	}
}

namespace
{
	FSLNeed NeedL(ESLNeedKind Kind, const TCHAR* Type, const TCHAR* Label, const TCHAR* Complaint, const TCHAR* Met,
		int32 MaxCells = 4, int32 MinCells = 4, int32 FamilyIndex = 0)
	{
		FSLNeed N;
		N.Kind = Kind;
		N.Type = Type;
		N.Label = Label;
		N.Complaint = Complaint;
		N.MetLine = Met;
		N.MaxCells = MaxCells;
		N.MinCells = MinCells;
		N.FamilyIndex = FamilyIndex;
		return N;
	}

	FSLTask OpenGoal(const TCHAR* Text, const TCHAR* Intro, std::initializer_list<FName> Unlocks)
	{
		FSLTask T;
		T.Text = Text;
		T.Speech = Intro;
		T.Kind = ESLTaskKind::FamiliesHappy;
		T.Unlocks = Unlocks;
		T.Speaker = 0;
		return T;
	}
}

ASLLevelDirector::ASLLevelDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics; // yerlestirme ayni karede gorulsun
	ConfigureLevel(1);
}

void ASLLevelDirector::ConfigureLevel(int32 Number)
{
	LevelNumber = Number;
	Tasks.Reset();
	Families.Reset();
	// Varsayilan: egitim zorlugu, hamle siniri yok, her hamle 1 saat.
	bApplyTuning = false;
	MoveLimit = 0;
	StartHour = 7.f;
	HoursPerMove = 1.f;
	bOpenLevel = false;
	bAutoAssign = true;
	RequiredTypes.Reset();
	SlotLimits.Reset();
	if (Number == 0)
	{
		ConfigureMenu();
		return;
	}
	if (Number == 3)
	{
		ConfigureLevel3();
		return;
	}
	if (Number == 2)
	{
		ConfigureLevel2();
		return;
	}

	// 1. bolum (egitim / vertical slice): aile gelir, eve yerlesir; hastane, AVM; yollarla baglanir.
	LevelNumber = 1;
	LevelTitle = TEXT("1. Bölüm: İlk mahalle");
	NextLevelNumber = 2;
	bNeighborsArrive = true;
	BusStopType = NAME_None;
	FinalSpeech = TEXT("Mahallemiz hazır. Teşekkürler başkan!");
	FSLFamilyDef Aile;
	Aile.Name = TEXT("Aile");
	Aile.WaitLocal = FVector(-230.f, -250.f, 0.f);
	Families.Add(Aile);
	Tasks.Add(MakeTask(TEXT("Aile için bir ev kur"),
		TEXT("Merhaba başkan! Bu şehre yeni taşındık ama evimiz yok. Bize bir ev yapar mısın?"),
		TEXT("Ne güzel bir ev!"),
		TEXT("Sağdaki çekmeceyi çek, Ev'i seç. Merkeze yakın koy; kapı yönünü R / Q çevirir."),
		ESLTaskKind::PlaceType, TEXT("Ev"), { TEXT("Ev") }));
	Tasks.Add(MakeTask(TEXT("Aile eve yerleşsin"),
		TEXT("Hemen eşyalarımızı taşıyoruz!"),
		TEXT("Yerleştik, çok rahat."),
		TEXT("Aile kapıya yürüyor, biraz bekle."),
		ESLTaskKind::FamilyMovedIn, TEXT("Ev"), {}));
	Tasks.Add(MakeTask(TEXT("Bir hastane kur"),
		TEXT("Çocuklar hastalanırsa nereye gideceğiz? Yakında bir hastane olsa..."),
		TEXT("İçimiz rahatladı."),
		TEXT("Hastaneyi evin karşı tarafına koy: ağırlıklar birbirini dengeler."),
		ESLTaskKind::PlaceType, TEXT("Hastane"), { TEXT("Hastane") }));
	Tasks.Add(MakeTask(TEXT("Bir alışveriş merkezi kur"),
		TEXT("Alışveriş için bir AVM çok iyi olurdu. Ama dikkat, AVM çok ağır!"),
		TEXT("Harika, alışveriş günü!"),
		TEXT("Koymadan önce bara bak: tahmin işareti kırmızıya geçiyorsa başka yer seç."),
		ESLTaskKind::PlaceType, TEXT("AVM"), { TEXT("AVM") }));
	FSLTask Roads = MakeTask(TEXT("Evi, hastaneyi ve AVM'yi yollarla bağla"),
		TEXT("Hastaneye ve AVM'ye yürüyerek gidemiyoruz. Yol yapar mısın?"),
		TEXT(""),
		TEXT("Yol, binaların kapı önündeki sarı karelere değmeli."),
		ESLTaskKind::ConnectRoads, TEXT("Yol"), { TEXT("Yol") });
	Roads.ConnectTypes = { TEXT("Ev"), TEXT("Hastane"), TEXT("AVM") };
	Tasks.Add(Roads);
}

void ASLLevelDirector::ConfigureLevel3()
{
	// 3. bolum, acik (16 Eki): "Is This Seat Taken?" gibi. Yilmazlar ve Demirler bastan burada; isteklerini
	// tiklayarak ogrenirsin. Aydinlar durak kurulunca otobusle gelir. Istekler birbirini zorlar; stok
	// sinirli; platform hafif; 24 hamle (her hamle 2 saat).
	LevelNumber = 3;
	LevelTitle = TEXT("3. Bölüm: Komşular");
	NextLevelNumber = 0;
	bNeighborsArrive = false;
	bOpenLevel = true;
	bAutoAssign = false;
	BusStopType = TEXT("Durak");
	FinalSpeech = TEXT("Herkes istediği yerde. Bu mahalle bir harika başkan!");
	bApplyTuning = true;
	TunePlatformMass = 8.f;
	TuneTiltPerCm = 0.10f;
	TuneFailSeconds = 1.2f;
	MoveLimit = 24;
	HoursPerMove = 2.f;
	RequiredTypes = { TEXT("Hastane"), TEXT("AVM"), TEXT("Park"), TEXT("Durak") };
	SlotLimits = { { TEXT("Ev"), 3 }, { TEXT("Hastane"), 1 }, { TEXT("AVM"), 1 }, { TEXT("Park"), 1 }, { TEXT("Durak"), 1 }, { TEXT("Yol"), 12 } };

	FSLFamilyDef Yilmaz;
	Yilmaz.Name = TEXT("Yılmaz ailesi");
	Yilmaz.WaitLocal = FVector(-230.f, -250.f, 0.f);
	Yilmaz.Tint = FLinearColor(0.45f, 0.35f, 0.70f);
	Yilmaz.bWishesHidden = true;
	Yilmaz.Greeting = TEXT("Biz yaşlıyız evladım. Hastane yürüme mesafesinde olsun, yolu da olsun.");
	Yilmaz.WaitLines = { TEXT("Dizlerim ağrıyor, bir ev bulsak..."), TEXT("Gençken burası tarlaydı."), TEXT("Torunlar ne zaman gelecek?") };
	Yilmaz.HappyLines = { TEXT("Çay demledim, buyur başkan."), TEXT("Torunlar yakında, ne güzel."), TEXT("Hastane yakın, içim rahat.") };
	Yilmaz.Needs.Add(NeedL(ESLNeedKind::NearType, TEXT("Hastane"), TEXT("Hastaneye yakın"),
		TEXT("Hastane çok uzak, bu yaşta yürüyemem! En fazla 3 birim."), TEXT("Hastane hemen şurada, oh be."), 3));
	Yilmaz.Needs.Add(NeedL(ESLNeedKind::ConnectedTo, TEXT("Hastane"), TEXT("Hastaneye yol"),
		TEXT("Hastane yakın ama yol yok, oraya nasıl gideceğiz?"), TEXT("Hastaneye yolumuz var.")));
	Families.Add(Yilmaz);

	FSLFamilyDef Demir;
	Demir.Name = TEXT("Demir ailesi");
	Demir.WaitLocal = FVector(-285.f, 150.f, 0.f);
	Demir.Tint = FLinearColor(0.85f, 0.38f, 0.35f);
	Demir.bWishesHidden = true;
	Demir.Greeting = TEXT("Çocuklarımız için parka yakın oturalım. Ama AVM'nin gürültüsünden uzak!");
	Demir.WaitLines = { TEXT("Çocuklar sıkıldı!"), TEXT("Bebek uyuyor, sessiz olalım."), TEXT("Park var mı burada?") };
	Demir.HappyLines = { TEXT("Çocuklar parkta, biz kahvede."), TEXT("Ne sessiz, ne güzel."), TEXT("Kaydıraktan yine düştü ama gülüyor.") };
	Demir.Needs.Add(NeedL(ESLNeedKind::NearType, TEXT("Park"), TEXT("Parka yakın"),
		TEXT("Park çok uzak, çocuklar nerede oynayacak? En fazla 2 birim."), TEXT("Park! Çocuklar bayılacak!"), 2));
	Demir.Needs.Add(NeedL(ESLNeedKind::FarFrom, TEXT("AVM"), TEXT("AVM'den uzak"),
		TEXT("AVM çok gürültülü, bebek uyuyamıyor! En az 4 birim uzak olalım."), TEXT("Ne güzel, sessiz sakin."), 4, 4));
	Families.Add(Demir);

	FSLFamilyDef Aydin;
	Aydin.Name = TEXT("Aydın ailesi");
	Aydin.WaitLocal = FVector(-285.f, 0.f, 0.f);
	Aydin.Tint = FLinearColor(0.25f, 0.60f, 0.45f);
	Aydin.ArriveWhenType = TEXT("Durak");
	Aydin.bArriveAtStop = true;
	Aydin.bWishesHidden = true;
	Aydin.Greeting = TEXT("Biz Yılmazların çocuklarıyız! Annemlerin yanında oturalım, işe de otobüsle gidiyoruz.");
	Aydin.WaitLines = { TEXT("Annemler nerede oturuyor?"), TEXT("Otobüs çok rahattı."), TEXT("Bu ada sallanıyor mu, bana mı öyle geliyor?") };
	Aydin.HappyLines = { TEXT("Annem yine börek yapmış!"), TEXT("İşe otobüsle on dakika."), TEXT("Komşuluk böyle olur.") };
	Aydin.Needs.Add(NeedL(ESLNeedKind::NearFamily, TEXT(""), TEXT("Yılmazlara yakın"),
		TEXT("Annemler çok uzakta kaldı! Evleri en fazla 2 birim olsun."), TEXT("Annemlerin yanındayız!"), 2, 0, 0));
	Aydin.Needs.Add(NeedL(ESLNeedKind::ConnectedTo, TEXT("Durak"), TEXT("Durağa yol"),
		TEXT("Evimizden durağa yol yok, işe nasıl gideceğiz?"), TEXT("Otobüse yürüyerek gidiyoruz.")));
	Families.Add(Aydin);

	Tasks.Add(OpenGoal(TEXT("Herkes mutlu olsun"),
		TEXT("Hoş geldin başkan! Burada herkesin bir isteği var. Üstümüze tıkla, anlatalım."),
		{ TEXT("Ev"), TEXT("Hastane"), TEXT("AVM"), TEXT("Yol"), TEXT("Park"), TEXT("Durak") }));
}


void ASLLevelDirector::ConfigureLevel2()
{
	// 2. bolum, acik (16 Eki): sirali gorev yok. Kaya ailesi bastan burada; istekleri tiklayinca acilir.
	// Durak konunca Demir ailesi otobusle gelir. Hedef: iki aile mutlu + hastane, AVM, durak kurulu.
	LevelNumber = 2;
	LevelTitle = TEXT("2. Bölüm: Otobüs durağı");
	NextLevelNumber = 3;
	bNeighborsArrive = false;
	bOpenLevel = true;
	bAutoAssign = false;
	BusStopType = TEXT("Durak");
	FinalSpeech = TEXT("İki aile de çok mutlu. Teşekkürler başkan!");
	bApplyTuning = true;
	TunePlatformMass = 10.f;
	TuneTiltPerCm = 0.09f;
	TuneFailSeconds = 1.3f;
	RequiredTypes = { TEXT("Hastane"), TEXT("AVM"), TEXT("Durak") };
	SlotLimits = { { TEXT("Ev"), 2 }, { TEXT("Hastane"), 1 }, { TEXT("AVM"), 1 }, { TEXT("Durak"), 1 }, { TEXT("Yol"), 16 } };

	FSLFamilyDef Kaya;
	Kaya.Name = TEXT("Kaya ailesi");
	Kaya.WaitLocal = FVector(-230.f, -250.f, 0.f);
	Kaya.Tint = FLinearColor(0.15f, 0.35f, 0.75f);
	Kaya.bWishesHidden = true;
	Kaya.Greeting = TEXT("Merhaba başkan! Evimizden hastaneye ve AVM'ye yol olsun, başka bir şey istemeyiz.");
	Kaya.WaitLines = { TEXT("Nereye yerleşeceğiz acaba?"), TEXT("Eşyalar ağır, çabuk olalım!"), TEXT("Burası çok güzel bir ada.") };
	Kaya.HappyLines = { TEXT("Bu mahalleyi çok sevdik."), TEXT("Akşam yürüyüşü yapalım mı?"), TEXT("Komşular çok tatlı.") };
	Kaya.Needs.Add(NeedL(ESLNeedKind::ConnectedTo, TEXT("Hastane"), TEXT("Hastaneye yol"),
		TEXT("Evimizden hastaneye yol yok. Çocuklar hastalanırsa ne yapacağız?"), TEXT("Hastaneye yolumuz var, içimiz rahat!")));
	Kaya.Needs.Add(NeedL(ESLNeedKind::ConnectedTo, TEXT("AVM"), TEXT("AVM'ye yol"),
		TEXT("Evimizden AVM'ye yol yok, alışverişe gidemiyoruz."), TEXT("AVM'ye yürüyerek gidebiliyoruz!")));
	Families.Add(Kaya);

	FSLFamilyDef Demir;
	Demir.Name = TEXT("Demir ailesi");
	Demir.WaitLocal = FVector(-285.f, 150.f, 0.f);
	Demir.Tint = FLinearColor(0.85f, 0.38f, 0.35f);
	Demir.ArriveWhenType = TEXT("Durak");
	Demir.bArriveAtStop = true;
	Demir.bWishesHidden = true;
	Demir.Greeting = TEXT("Selam! Her gün otobüsle işe gidiyoruz. Evimiz durağa yakın olsun, kapımızdan durağa yol gitsin.");
	Demir.WaitLines = { TEXT("Otobüs yolculuğu uzundu!"), TEXT("Durağa yakın bir ev, lütfen."), TEXT("Burada kim oturuyor?") };
	Demir.HappyLines = { TEXT("Otobüs tam vaktinde!"), TEXT("Ne huzurlu bir yer."), TEXT("İşe yetiştik bugün de.") };
	Demir.Needs.Add(NeedL(ESLNeedKind::NearType, TEXT("Durak"), TEXT("Durağa yakın"),
		TEXT("Durak çok uzak, otobüse yetişemiyoruz! En fazla 4 birim olmalı."), TEXT("Durak hemen yanımızda, harika!"), 4));
	Demir.Needs.Add(NeedL(ESLNeedKind::ConnectedTo, TEXT("Durak"), TEXT("Durağa yol"),
		TEXT("Kapımızdan durağa yol yok!"), TEXT("Kapıdan durağa yol var!")));
	Families.Add(Demir);

	Tasks.Add(OpenGoal(TEXT("Herkes mutlu olsun"),
		TEXT("Hoş geldin başkan! Önce bizimle tanış: üstümüze tıkla."),
		{ TEXT("Ev"), TEXT("Hastane"), TEXT("AVM"), TEXT("Yol"), TEXT("Durak") }));
}

ASLPlatform* ASLLevelDirector::GetPlatform() const
{
	ASLGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ASLGameMode>() : nullptr;
	return GM ? GM->GetPlatform() : nullptr;
}

USLBuildComponent* ASLLevelDirector::GetBuild() const
{
	const ASLPlayerController* PC = GetWorld() ? Cast<ASLPlayerController>(GetWorld()->GetFirstPlayerController()) : nullptr;
	return PC ? PC->Build.Get() : nullptr;
}

float ASLLevelDirector::GetTimeHours() const
{
	if (LevelNumber == 0 && GetWorld())
	{
		return StartHour + GetWorld()->GetTimeSeconds() * MenuHoursPerSecond;
	}
	return StartHour + GetMoveCount() * HoursPerMove;
}

void ASLLevelDirector::ConfigureMenu()
{
	// Ana menu (16 Eki): arkada canli bir kasaba. 3. bolumun cozumu kurulu, uc aile evlerine yurur,
	// otobus calisir, gun dongusu akar; gorev, istek ve hamle yok.
	LevelNumber = 0;
	LevelTitle = TEXT("Sallantı");
	NextLevelNumber = 1;
	bNeighborsArrive = false;
	bOpenLevel = false;
	bAutoAssign = true;
	BusStopType = TEXT("Durak");
	FinalSpeech.Reset();
	StartHour = 9.f; // menu gunduz acilsin; 0.15 saat/sn ile aksam ~1,5 dk sonra
	const FLinearColor Tints[3] = { FLinearColor(0.45f, 0.35f, 0.70f), FLinearColor(0.85f, 0.38f, 0.35f), FLinearColor(0.25f, 0.60f, 0.45f) };
	const FVector Waits[3] = { FVector(-285.f, -60.f, 0.f), FVector(-285.f, 150.f, 0.f), FVector(-230.f, -250.f, 0.f) };
	for (int32 I = 0; I < 3; ++I)
	{
		FSLFamilyDef F;
		F.Name = FString::Printf(TEXT("Aile %d"), I + 1);
		F.WaitLocal = Waits[I];
		F.Tint = Tints[I];
		Families.Add(F);
	}
}

void ASLLevelDirector::SpawnMenuTown()
{
	USLBuildComponent* B = GetBuild();
	if (bMenuSpawned || !B)
	{
		return;
	}
	bMenuSpawned = true;
	B->bBuildEnabled = false;
	// 3. bolumun test cozumu: dengede (egim ~2 derece), herkes mutlu olurdu.
	B->SpawnDirect(TEXT("Hastane"), FIntPoint(4, 6), 3);
	B->SpawnDirect(TEXT("AVM"), FIntPoint(7, 5), 3);
	B->SpawnDirect(TEXT("Park"), FIntPoint(2, 9), 3);
	B->SpawnDirect(TEXT("Ev"), FIntPoint(3, 5), 0);
	B->SpawnDirect(TEXT("Ev"), FIntPoint(4, 10), 3);
	B->SpawnDirect(TEXT("Ev"), FIntPoint(3, 3), 0);
	B->SpawnDirect(TEXT("Yol"), FIntPoint(4, 5), 0);
	for (int32 X = 4; X <= 8; ++X)
	{
		B->SpawnDirect(TEXT("Yol"), FIntPoint(X, 4), 0);
	}
	B->SpawnDirect(TEXT("Yol"), FIntPoint(4, 3), 0);
	B->SpawnDirect(TEXT("Durak"), FIntPoint(6, 3), 1);
	UE_LOG(LogSLLevel, Log, TEXT("SL_MENU kasaba kuruldu"));
}

int32 ASLLevelDirector::GetMoveCount() const
{
	const USLBuildComponent* B = GetBuild();
	return B ? B->GetMoveCount() : 0;
}

void ASLLevelDirector::ApplyTuning()
{
	ASLPlatform* P = GetPlatform();
	if (!bApplyTuning || !P)
	{
		return;
	}
	P->PlatformMass = TunePlatformMass;
	P->TiltPerCm = TuneTiltPerCm;
	P->FailSeconds = TuneFailSeconds;
	UE_LOG(LogSLLevel, Log, TEXT("SL_TUNING bolum %d: PlatformMass %.0f TiltPerCm %.2f FailSeconds %.1f hamle siniri %d"),
		LevelNumber, TunePlatformMass, TuneTiltPerCm, TuneFailSeconds, MoveLimit);
}

void ASLLevelDirector::CheckMoveLimit(float DeltaSeconds)
{
	if (MoveLimit <= 0 || bLevelComplete || GetMoveCount() < MoveLimit)
	{
		OutOfMovesTimer = 0.f;
		return;
	}
	USLBuildComponent* B = GetBuild();
	if (B && B->bBuildEnabled)
	{
		B->DisableBuilding(); // hamle kalmadi: elindeki eski yerine doner
	}
	// Yuruyen aile varsa bekle: son hamle bolumu bitiriyor olabilir.
	const ASLPlatform* P = GetPlatform();
	bool bWalking = false;
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : P ? P->GetPlaceables() : TArray<TWeakObjectPtr<ASLPlaceable>>())
	{
		const ASLPlaceable* H = Weak.Get();
		bWalking |= H && H->HasResidentsStarted() && !H->bResidentsArrived;
	}
	OutOfMovesTimer = bWalking ? 0.f : OutOfMovesTimer + DeltaSeconds;
	if (OutOfMovesTimer > 1.0f)
	{
		if (ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>())
		{
			GM->HandleOutOfMoves();
		}
	}
}

int32 ASLLevelDirector::FootDistance(FIntPoint MinA, FIntPoint FootA, FIntPoint MinB, FIntPoint FootB)
{
	const int32 Dx = FMath::Max3(0, MinB.X - (MinA.X + FootA.X - 1), MinA.X - (MinB.X + FootB.X - 1));
	const int32 Dy = FMath::Max3(0, MinB.Y - (MinA.Y + FootA.Y - 1), MinA.Y - (MinB.Y + FootB.Y - 1));
	return Dx + Dy;
}

void ASLLevelDirector::BeginPlay()
{
	Super::BeginPlay();
	FamilyActors.SetNum(Families.Num());
	Homes.SetNum(Families.Num());
	Happiness.Init(0.f, Families.Num());
	DisplayHappiness.Init(0.f, Families.Num());
	Discovered.Init(false, Families.Num());
	for (int32 I = 0; I < Families.Num(); ++I)
	{
		Discovered[I] = !Families[I].bWishesHidden;
	}
	NeedMet.SetNum(Families.Num());
	NeedInit.Init(false, Families.Num());
	BubbleText.SetNum(Families.Num());
	BubbleUntil.Init(-1.f, Families.Num());
	NextChat.Init(0.f, Families.Num());
	for (int32 I = 0; I < Families.Num(); ++I)
	{
		NextChat[I] = 4.f + 3.f * I;
	}
	ApplyTuning();
	if (USLBuildComponent* B = GetBuild())
	{
		B->Limits = SlotLimits;
	}
	UE_LOG(LogSLLevel, Log, TEXT("SL_LEVEL %d %s, %d gorev, %d aile"), LevelNumber, *LevelTitle, Tasks.Num(), Families.Num());
}

void ASLLevelDirector::UpdateConnectionFx()
{
	ASLPlatform* P = GetPlatform();
	if (!P)
	{
		return;
	}
	auto RoadAt = [P](const FIntPoint& C) -> ASLPlaceable*
	{
		ASLPlaceable* R = P->FindPlaceableAt(C);
		return (R && R->bFlat) ? R : nullptr;
	};
	TSet<TWeakObjectPtr<ASLPlaceable>> Now;
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : P->GetPlaceables())
	{
		ASLPlaceable* B = Weak.Get();
		if (!B || B->bHeld || !B->bHasEntrance)
		{
			continue;
		}
		ASLPlaceable* Door = nullptr;
		for (const FIntPoint& E : B->GetEntranceCells())
		{
			Door = Door ? Door : RoadAt(E);
		}
		if (B->IsEntranceConnected() != (Door != nullptr))
		{
			B->SetEntranceConnected(Door != nullptr); // kapidan yola giris yolu
		}
		if (!Door)
		{
			continue;
		}
		Now.Add(B);
		if (ConnectedBuildings.Contains(B))
		{
			continue;
		}
		// Yeni baglanti: bina halka + parlama; yol kapidan uzaklastikca sirayla parlar (akis).
		B->PlayConnectFx(0.f, true);
		TMap<ASLPlaceable*, int32> Dist;
		TArray<ASLPlaceable*> Queue = { Door };
		Dist.Add(Door, 0);
		for (int32 Q = 0; Q < Queue.Num() && Q < 400; ++Q)
		{
			ASLPlaceable* R = Queue[Q];
			R->PlayConnectFx(0.07f * Dist[R], false);
			const FIntPoint Dirs[4] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1} };
			for (const FIntPoint& D : Dirs)
			{
				if (ASLPlaceable* N = RoadAt(R->CellMin + D); N && !Dist.Contains(N))
				{
					Dist.Add(N, Dist[R] + 1);
					Queue.Add(N);
				}
			}
		}
		UE_LOG(LogSLLevel, Log, TEXT("SL_CONNECT %s yola baglandi, %d yol karosu parladi"), *B->TypeId.ToString(), Queue.Num());
		ASLGameMode::PlaySfx(this, TEXT("SFX_Baglanti"), 3);
	}
	ConnectedBuildings = MoveTemp(Now);
}


// --- Aileler ---------------------------------------------------------------------------------------

void ASLLevelDirector::SpawnFamilies()
{
	ASLPlatform* P = GetPlatform();
	if (!P)
	{
		return;
	}
	for (int32 I = 0; I < Families.Num() && I < FamilyActors.Num(); ++I)
	{
		const FSLFamilyDef& Def = Families[I];
		if (FamilyActors[I])
		{
			continue;
		}
		if (!Def.ArriveWhenType.IsNone())
		{
			if (!IsRequiredPlaced(Def.ArriveWhenType))
			{
				continue;
			}
		}
		else if (!bOpenLevel && CurrentTask < Def.ArriveAtTask && !bLevelComplete)
		{
			continue;
		}
		FVector Wait = Def.WaitLocal;
		if (Def.bArriveAtStop && !BusStopType.IsNone())
		{
			// Otobusle geldi: duragin plakasinda, yola bakan kenarda belirir.
			for (const TWeakObjectPtr<ASLPlaceable>& Weak : P->GetPlaceables())
			{
				const ASLPlaceable* S = Weak.Get();
				if (S && !S->bHeld && S->TypeId == BusStopType)
				{
					const FIntPoint F = S->GetFacingDir();
					Wait = S->GetRootComponent()->GetRelativeLocation() + FVector(F.X, F.Y, 0.f) * 0.3f * P->GridSize;
					Wait.Z = 0.f;
					break;
				}
			}
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASLFamily* F = GetWorld()->SpawnActor<ASLFamily>(ASLFamily::StaticClass(), FTransform::Identity, Params);
		if (F)
		{
			// Platforma bagli: platform egilince aile de egilir, kutlesi yok.
			F->Tint = Def.Tint;
			F->AttachToComponent(P->TiltPivot, FAttachmentTransformRules::KeepRelativeTransform);
			F->SetActorRelativeLocation(Wait);
			F->BuildFigures();
			UE_LOG(LogSLLevel, Log, TEXT("SL_FAMILY %s geldi (%.0f,%.0f)"), *Def.Name, Wait.X, Wait.Y);
		}
		FamilyActors[I] = F;
		if (bOpenLevel && Def.bWishesHidden && I > 0)
		{
			Say(I, TEXT("Merhaba! Yeni geldik, bize bir tıklar mısın?"));
		}
	}
}

bool ASLLevelDirector::IsFamilyPresent(int32 I) const
{
	return FamilyActors.IsValidIndex(I) && FamilyActors[I] != nullptr;
}

int32 ASLLevelDirector::FindFamilyOfHome(const ASLPlaceable* House) const
{
	for (int32 I = 0; I < Homes.Num(); ++I)
	{
		if (House && Homes[I].Get() == House)
		{
			return I;
		}
	}
	return INDEX_NONE;
}

bool ASLLevelDirector::GetFamilyAnchor(int32 I, FVector& Out) const
{
	const ASLFamily* F = GetFamilyActor(I);
	if (!F || F->IsHidden())
	{
		return false;
	}
	Out = F->GetSpeechAnchor();
	return true;
}

bool ASLLevelDirector::GetSpeech(FString& OutText, FVector& OutWorldAnchor) const
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	int32 Speaker = 0;
	if (bLevelComplete)
	{
		OutText = FinalSpeech;
		Speaker = Families.Num() - 1;
	}
	else if (Now < DoneUntil && !DoneText.IsEmpty())
	{
		OutText = DoneText;
		Speaker = DoneSpeaker;
	}
	else if (bOpenLevel)
	{
		// Acik bolum: yalnizca giris cumlesi, ilk aile secilene kadar. Gerisi ailelerin kendi balonlari.
		if (!bIntroOver && Tasks.IsValidIndex(0))
		{
			OutText = Tasks[0].Speech;
			Speaker = 0;
		}
	}
	else if (Tasks.IsValidIndex(CurrentTask))
	{
		const FSLTask& T = Tasks[CurrentTask];
		OutText = T.Speech;
		Speaker = T.Speaker;
		if (T.Kind == ESLTaskKind::FamiliesHappy)
		{
			// Son gorev: mutsuz ilk aile sikayet eder (karsilanmayan ilk istegi).
			OutText.Reset();
			for (int32 I = 0; I < Families.Num() && OutText.IsEmpty(); ++I)
			{
				if (IsFamilyHappy(I) || !GetHome(I))
				{
					continue;
				}
				for (const FSLNeed& N : Families[I].Needs)
				{
					if (EvaluateNeed(N, GetHome(I), nullptr) < 0.999f)
					{
						OutText = N.Complaint;
						Speaker = I;
						break;
					}
				}
			}
		}
	}
	if (OutText.IsEmpty())
	{
		return false;
	}
	// Konusan aile bekliyorsa onun ustunden, eve yerlestiyse evin ustunden; olmazsa ilk uygun aile.
	if (GetFamilyAnchor(Speaker, OutWorldAnchor))
	{
		return true;
	}
	for (int32 I = 0; I < Families.Num(); ++I)
	{
		if (GetFamilyAnchor(I, OutWorldAnchor))
		{
			return true;
		}
	}
	return false;
}

void ASLLevelDirector::StartFamilies()
{
	const ASLPlatform* P = GetPlatform();
	if (!P)
	{
		return;
	}
	// Aileler: evi yoksa bekledigi yerde, evine yuruyorsa yolda, vardiysa kapinin onunde. Ev elde ise gizli.
	for (int32 I = 0; I < FamilyActors.Num(); ++I)
	{
		ASLFamily* F = FamilyActors[I];
		if (!F)
		{
			continue;
		}
		ASLPlaceable* Home = Homes[I].Get();
		if (Homes[I].IsStale() || (Homes[I].IsValid() && !Home))
		{
			Homes[I] = nullptr; // ev geri alindi: aile tekrar bekler
			Home = nullptr;
			F->GetRootComponent()->SetRelativeLocation(Families[I].WaitLocal);
		}
		if (Home && Home->bHeld)
		{
			F->SetWaitingVisible(false);
			continue;
		}
		F->SetWaitingVisible(true);
		if (Home && F->ConsumeArrived())
		{
			Home->MarkFamilyArrived();
			F->Hop();
			UE_LOG(LogSLLevel, Log, TEXT("SL_FAMILY %s eve yerlesti (%d,%d)"), *Families[I].Name, Home->CellMin.X, Home->CellMin.Y);
		}
		if (Home && Home->bResidentsArrived && !F->IsWalking())
		{
			// Ev tasindiysa aile de yeni kapinin onune.
			F->GetRootComponent()->SetRelativeLocation(Home->GetDoorSpotLocal());
			F->GetRootComponent()->SetRelativeRotation(FRotator(0, Home->GetDoorSpotYaw(), 0));
		}
		else if (Home && !Home->bResidentsArrived && !F->IsWalking())
		{
			F->WalkPath(Home->GetArrivalPath(F->GetRootComponent()->GetRelativeLocation()), Home->GetDoorSpotYaw());
		}
	}
	// Bos evler: 1. bolumde bekleyen ilk aile en son konan bos eve kendiliginden tasinir; acik bolumde oyuncu secer.
	const TArray<TWeakObjectPtr<ASLPlaceable>>& All = P->GetPlaceables();
	for (int32 K = All.Num() - 1; K >= 0; --K)
	{
		ASLPlaceable* House = All[K].Get();
		if (!House || House->bHeld || House->Residents <= 0 || House->HasResidentsStarted())
		{
			continue;
		}
		int32 Fam = INDEX_NONE;
		for (int32 I = 0; bAutoAssign && I < FamilyActors.Num() && Fam == INDEX_NONE; ++I)
		{
			if (FamilyActors[I] && !Homes[I].IsValid())
			{
				Fam = I;
			}
		}
		if (Fam != INDEX_NONE)
		{
			AssignFamilyToHouse(Fam, House);
		}
		else if (bNeighborsArrive && !bOpenLevel)
		{
			// Ailesi olmayan ev: kapinin baktigi yondeki kenardan gelen yeni komsular (evin kendi figurleri).
			const FVector C = House->GetRootComponent()->GetRelativeLocation();
			const float H = P->HalfExtent - 20.f;
			const FIntPoint D = House->GetFacingDir();
			House->StartResidentsArrival(D.X != 0 ? FVector(D.X * H, C.Y, 0.f) : FVector(C.X, D.Y * H, 0.f));
		}
	}
}

bool ASLLevelDirector::AssignFamilyToHouse(int32 I, ASLPlaceable* House)
{
	ASLFamily* F = GetFamilyActor(I);
	if (!F || !House || House->Residents <= 0 || House->bHeld)
	{
		return false;
	}
	if (Homes[I].Get() == House)
	{
		return true;
	}
	if (House->HasResidentsStarted())
	{
		return false; // baska ailenin evi
	}
	if (ASLPlaceable* Old = Homes[I].Get())
	{
		Old->MarkFamilyAssigned(false);
	}
	Homes[I] = House;
	NeedInit[I] = false;
	House->MarkFamilyAssigned(true);
	House->ResidentTint = Families[I].Tint;
	F->WalkPath(House->GetArrivalPath(F->GetRootComponent()->GetRelativeLocation()), House->GetDoorSpotYaw());
	if (bOpenLevel)
	{
		Say(I, TEXT("Hemen taşınıyoruz!"));
	}
	ASLGameMode::PlaySfx(this, TEXT("SFX_Konusma"), 0);
	UE_LOG(LogSLLevel, Log, TEXT("SL_FAMILY %s eve yuruyor (%d,%d)"), *Families[I].Name, House->CellMin.X, House->CellMin.Y);
	return true;
}

void ASLLevelDirector::SelectFamily(int32 I)
{
	if (!IsFamilyPresent(I))
	{
		return;
	}
	const bool bFirst = !Discovered[I];
	Discovered[I] = true;
	SelectedFamily = I;
	bIntroOver = true;
	if (ASLFamily* F = GetFamilyActor(I))
	{
		F->Hop();
	}
	if (bFirst && !Families[I].Greeting.IsEmpty())
	{
		Say(I, Families[I].Greeting, BubbleSeconds + 2.f);
	}
	else if (!GetHome(I))
	{
		Say(I, TEXT("Bize bir ev kur ya da boş bir eve tıkla, hemen taşınalım."));
	}
	ASLGameMode::PlaySfx(this, TEXT("SFX_Konusma"), 1);
	UE_LOG(LogSLLevel, Log, TEXT("SL_SELECT %s (istekler %s)"), *Families[I].Name, bFirst ? TEXT("ilk kez acildi") : TEXT("acik"));
}

void ASLLevelDirector::Say(int32 I, const FString& Text, float Seconds)
{
	if (!BubbleText.IsValidIndex(I) || Text.IsEmpty())
	{
		return;
	}
	BubbleText[I] = Text;
	BubbleUntil[I] = GetWorld()->GetTimeSeconds() + (Seconds > 0.f ? Seconds : BubbleSeconds);
	NextChat[I] = BubbleUntil[I] + FMath::FRandRange(ChatterSeconds.X, ChatterSeconds.Y);
}

bool ASLLevelDirector::GetFamilyBubble(int32 I, FString& Out) const
{
	if (!BubbleText.IsValidIndex(I) || !GetWorld() || GetWorld()->GetTimeSeconds() > BubbleUntil[I])
	{
		return false;
	}
	Out = BubbleText[I];
	return !Out.IsEmpty();
}

bool ASLLevelDirector::IsNeedMet(int32 Fam, int32 Need) const
{
	return NeedMet.IsValidIndex(Fam) && NeedMet[Fam].IsValidIndex(Need) && NeedMet[Fam][Need];
}

FString ASLLevelDirector::DescribeNeed(int32 Fam, int32 Need, bool& bOutMet) const
{
	bOutMet = false;
	if (!Families.IsValidIndex(Fam) || !Families[Fam].Needs.IsValidIndex(Need) || !GetHome(Fam))
	{
		return FString();
	}
	FString Detail;
	bOutMet = EvaluateNeed(Families[Fam].Needs[Need], GetHome(Fam), &Detail) >= 0.999f;
	return bOutMet ? FString(TEXT("tamam")) : Detail;
}

bool ASLLevelDirector::IsRequiredPlaced(FName Type) const
{
	const ASLPlatform* P = GetPlatform();
	for (const TWeakObjectPtr<ASLPlaceable>& W : P ? P->GetPlaceables() : TArray<TWeakObjectPtr<ASLPlaceable>>())
	{
		if (W.Get() && !W->bHeld && W->TypeId == Type)
		{
			return true;
		}
	}
	return false;
}

FString ASLLevelDirector::DescribeOpenGoal() const
{
	TArray<FString> Missing;
	const USLBuildComponent* B = GetBuild();
	for (const FName& T : RequiredTypes)
	{
		if (!IsRequiredPlaced(T))
		{
			const int32 S = B ? B->FindSlotById(T) : INDEX_NONE;
			Missing.Add(B && B->Slots.IsValidIndex(S) ? B->Slots[S].Label : T.ToString());
		}
	}
	int32 Happy = 0;
	for (int32 I = 0; I < Families.Num(); ++I)
	{
		Happy += IsFamilyHappy(I) ? 1 : 0;
	}
	FString Out = FString::Printf(TEXT("Mutlu aile %d / %d"), Happy, Families.Num());
	if (Missing.Num())
	{
		Out += TEXT(" · Eksik: ") + FString::Join(Missing, TEXT(", "));
	}
	return Out;
}

void ASLLevelDirector::UpdateChatter(float DeltaSeconds)
{
	if (!bOpenLevel || bLevelComplete || !GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	// Gece/sabah selami: saat donumunde bir aile soyler.
	const float Day = ASLAmbience::DaylightAt(GetTimeHours());
	const bool bNight = Day < 0.3f;
	if (bNight != bWasNight)
	{
		bWasNight = bNight;
		for (int32 I = 0; I < FamilyActors.Num(); ++I)
		{
			if (GetHome(I))
			{
				Say(I, bNight ? TEXT("İyi geceler komşular!") : TEXT("Günaydın! Ne güzel bir sabah."));
				break;
			}
		}
	}
	for (int32 I = 0; I < FamilyActors.Num(); ++I)
	{
		if (!FamilyActors[I] || Now < NextChat[I] || Now < BubbleUntil[I])
		{
			continue;
		}
		const FSLFamilyDef& Def = Families[I];
		FString Line;
		const ASLPlaceable* Home = GetHome(I);
		if (!Discovered[I])
		{
			Line = TEXT("Psst, başkan! Bize tıkla, ne istediğimizi anlatalım.");
		}
		else if (!Home)
		{
			Line = Def.WaitLines.Num() ? Def.WaitLines[FMath::RandRange(0, Def.WaitLines.Num() - 1)] : FString(TEXT("Bir ev bulsak..."));
		}
		else if (!IsFamilyHappy(I))
		{
			for (int32 N = 0; N < Def.Needs.Num() && Line.IsEmpty(); ++N)
			{
				Line = IsNeedMet(I, N) ? FString() : Def.Needs[N].Complaint;
			}
		}
		else if (Def.HappyLines.Num())
		{
			Line = Def.HappyLines[FMath::RandRange(0, Def.HappyLines.Num() - 1)];
		}
		if (Line.IsEmpty())
		{
			NextChat[I] = Now + 5.f;
			continue;
		}
		Say(I, Line);
		ASLGameMode::PlaySfx(this, TEXT("SFX_Konusma"), 0, 0.5f, FMath::FRandRange(0.9f, 1.15f));
	}
}

int32 ASLLevelDirector::DistanceToType(FIntPoint CellMin, FIntPoint Foot, FName Type, const ASLPlaceable* Ignore) const
{
	const ASLPlatform* P = GetPlatform();
	int32 Best = -1;
	if (!P)
	{
		return Best;
	}
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : P->GetPlaceables())
	{
		const ASLPlaceable* B = Weak.Get();
		if (!B || B->bHeld || B == Ignore || B->TypeId != Type)
		{
			continue;
		}
		// Iki dikdortgen arasi en kisa hucre farki, sehir blogu: |dx| + |dy|. Komsu hucre = 1.
		const int32 D = FootDistance(CellMin, Foot, B->CellMin, B->GetFootprint());
		Best = Best < 0 ? D : FMath::Min(Best, D);
	}
	return Best;
}

float ASLLevelDirector::EvaluateNeed(const FSLNeed& Need, const ASLPlaceable* Home, FString* OutDetail, const ASLPlaceable* Ignore) const
{
	const ASLPlatform* P = GetPlatform();
	const USLBuildComponent* B = GetBuild();
	if (!P || !Home)
	{
		return 0.f;
	}
	const int32 Slot = B ? B->FindSlotById(Need.Type) : INDEX_NONE;
	const FString TypeLabel = (B && B->Slots.IsValidIndex(Slot)) ? B->Slots[Slot].Label : Need.Type.ToString();
	switch (Need.Kind)
	{
	case ESLNeedKind::ConnectedTo:
	{
		const TSet<FName> Types = P->TypesOnRoadNetwork(Home->GetEntranceCells(), Ignore);
		if (Types.Contains(Need.Type))
		{
			return 1.f;
		}
		if (OutDetail)
		{
			*OutDetail = Need.Label + TEXT(" yok");
		}
		return 0.f;
	}
	case ESLNeedKind::NearType:
	{
		const int32 D = DistanceToType(Home->CellMin, Home->GetFootprint(), Need.Type, Ignore);
		if (D < 0)
		{
			if (OutDetail)
			{
				*OutDetail = FString::Printf(TEXT("%s yok"), *TypeLabel);
			}
			return 0.f;
		}
		if (D <= Need.MaxCells)
		{
			return 1.f;
		}
		if (OutDetail)
		{
			*OutDetail = FString::Printf(TEXT("%s %d birim uzakta (en fazla %d)"), *TypeLabel, D, Need.MaxCells);
		}
		// Esigin otesinde mutsuz; yine de yaklastikca bar biraz dolsun (oyuncu dogru yonu gorsun).
		return FMath::Clamp(0.45f - 0.1f * (D - Need.MaxCells - 1), 0.1f, 0.45f);
	}
	case ESLNeedKind::FarFrom:
	{
		const int32 D = DistanceToType(Home->CellMin, Home->GetFootprint(), Need.Type, Ignore);
		if (D < 0 || D >= Need.MinCells)
		{
			return 1.f;
		}
		if (OutDetail)
		{
			*OutDetail = FString::Printf(TEXT("%s çok yakın: %d birim (en az %d)"), *TypeLabel, D, Need.MinCells);
		}
		return FMath::Clamp(0.45f * D / FMath::Max(1, Need.MinCells), 0.1f, 0.45f);
	}
	case ESLNeedKind::NearFamily:
	{
		const ASLPlaceable* Other = GetHome(Need.FamilyIndex);
		const FString Who = Families.IsValidIndex(Need.FamilyIndex) ? Families[Need.FamilyIndex].Name : FString(TEXT("Komşu"));
		if (!Other || Other == Home || Other->bHeld || Other == Ignore)
		{
			if (OutDetail)
			{
				*OutDetail = FString::Printf(TEXT("%s evi yok"), *Who);
			}
			return 0.f;
		}
		const int32 D = FootDistance(Home->CellMin, Home->GetFootprint(), Other->CellMin, Other->GetFootprint());
		if (D <= Need.MaxCells)
		{
			return 1.f;
		}
		if (OutDetail)
		{
			*OutDetail = FString::Printf(TEXT("%s %d birim uzakta (en fazla %d)"), *Who, D, Need.MaxCells);
		}
		return FMath::Clamp(0.45f - 0.1f * (D - Need.MaxCells - 1), 0.1f, 0.45f);
	}
	}
	return 0.f;
}

void ASLLevelDirector::UpdateHappiness(float DeltaSeconds)
{
	for (int32 I = 0; I < Families.Num() && I < Happiness.Num(); ++I)
	{
		const ASLPlaceable* Home = GetHome(I);
		if (!IsFamilyPresent(I) || !Home)
		{
			Happiness[I] = 0.f;
		}
		else if (!Home->bHeld) // tasinirken eski degeri koru
		{
			float Sum = 0.f;
			TArray<bool> Met;
			for (const FSLNeed& N : Families[I].Needs)
			{
				const float V = EvaluateNeed(N, Home, nullptr);
				Sum += V;
				Met.Add(V >= 0.999f);
			}
			Happiness[I] = Families[I].Needs.Num() ? Sum / Families[I].Needs.Num() : 1.f;
			// Istek degisince aile tepki verir: karsilandi -> sevinir, bozuldu -> sikayet (yalniz evdeyken).
			if (Home->bResidentsArrived)
			{
				if (NeedInit[I] && bOpenLevel)
				{
					for (int32 N = 0; N < Met.Num(); ++N)
					{
						if (NeedMet[I].IsValidIndex(N) && Met[N] != NeedMet[I][N])
						{
							Say(I, Met[N] ? Families[I].Needs[N].MetLine : Families[I].Needs[N].Complaint);
							break;
						}
					}
				}
				NeedInit[I] = true;
			}
			NeedMet[I] = Met;
		}
		if (ASLFamily* F = GetFamilyActor(I))
		{
			// Evsiz aile "notr" (ne zipliyor ne bas egik); evdeyse mutluluguna gore.
			F->SetMood(Home ? Happiness[I] : 0.6f, SelectedFamily == I, HoveredFamily == I);
		}
		const bool bWasHappy = DisplayHappiness[I] >= 0.999f;
		DisplayHappiness[I] = FMath::FInterpConstantTo(DisplayHappiness[I], Happiness[I], DeltaSeconds, HappinessLerpSpeed);
		if (!bWasHappy && DisplayHappiness[I] >= 0.999f && Families[I].Needs.Num() > 0)
		{
			UE_LOG(LogSLLevel, Log, TEXT("SL_HAPPY %s mutlu"), *Families[I].Name);
		}
	}
}

bool ASLLevelDirector::HasFamilyNeeds() const
{
	for (const FSLFamilyDef& F : Families)
	{
		if (F.Needs.Num() > 0)
		{
			return true;
		}
	}
	return false;
}

bool ASLLevelDirector::IsFamilyHappy(int32 I) const
{
	return IsFamilyPresent(I) && GetHome(I) && GetHome(I)->bResidentsArrived && GetHappiness(I) >= 0.999f;
}

FString ASLLevelDirector::GetFamilyStatus(int32 I) const
{
	if (!IsFamilyPresent(I))
	{
		return TEXT("Henüz gelmedi");
	}
	const ASLPlaceable* Home = GetHome(I);
	if (!Home)
	{
		return TEXT("Ev bekliyor");
	}
	if (Home->bHeld)
	{
		return TEXT("Evi taşınıyor");
	}
	for (const FSLNeed& N : Families[I].Needs)
	{
		FString Detail;
		if (EvaluateNeed(N, Home, &Detail) < 0.999f)
		{
			return Detail;
		}
	}
	return TEXT("Mutlu");
}

int32 ASLLevelDirector::FamilyForPlacement(const ASLPlaceable* Moving) const
{
	// Kimin evi olacak: tasinan evin ailesi; sonra secili aile; 1. bolumde evi olmayan ilk aile.
	// Acik bolumde istekleri bilinmeyen ailenin bolgesi gosterilmez (once tikla, ogren).
	int32 Fam = FindFamilyOfHome(Moving);
	if (Fam == INDEX_NONE && IsFamilyPresent(SelectedFamily))
	{
		Fam = SelectedFamily;
	}
	for (int32 I = 0; bAutoAssign && I < Families.Num() && Fam == INDEX_NONE; ++I)
	{
		if (IsFamilyPresent(I) && !GetHome(I))
		{
			Fam = I;
		}
	}
	return (Fam != INDEX_NONE && IsFamilyDiscovered(Fam)) ? Fam : INDEX_NONE;
}

bool ASLLevelDirector::DescribeHomePlacement(const ASLPlaceable* Ghost, const ASLPlaceable* Moving, FString& Out, bool& bGood) const
{
	if (!Ghost || Ghost->Residents <= 0)
	{
		return false;
	}
	const int32 Fam = FamilyForPlacement(Moving);
	if (Fam == INDEX_NONE || Families[Fam].Needs.Num() == 0)
	{
		return false;
	}
	// Hayalet ev buradaymis gibi butun istekleri tart (yol istegi hayaletin kapi onundeki agdan).
	TArray<FString> Bad;
	for (const FSLNeed& N : Families[Fam].Needs)
	{
		FString Detail;
		if (EvaluateNeed(N, Ghost, &Detail, Moving) < 0.999f)
		{
			Bad.Add(Detail);
		}
	}
	bGood = Bad.Num() == 0;
	Out = bGood
		? FString::Printf(TEXT("%s burada mutlu olur."), *Families[Fam].Name)
		: FString::Printf(TEXT("%s burada mutsuz: %s."), *Families[Fam].Name, *FString::Join(Bad, TEXT(", ")));
	return true;
}

bool ASLLevelDirector::CellSatisfiesDistanceNeeds(int32 Fam, FIntPoint Cell, const ASLPlaceable* Ignore) const
{
	bool bAny = false;
	for (const FSLNeed& N : Families[Fam].Needs)
	{
		int32 D = -1;
		switch (N.Kind)
		{
		case ESLNeedKind::NearType:
			bAny = true;
			D = DistanceToType(Cell, FIntPoint(1, 1), N.Type, Ignore);
			if (D < 1 || D > N.MaxCells)
			{
				return false;
			}
			break;
		case ESLNeedKind::FarFrom:
			bAny = true;
			D = DistanceToType(Cell, FIntPoint(1, 1), N.Type, Ignore);
			if (D >= 0 && D < N.MinCells)
			{
				return false;
			}
			break;
		case ESLNeedKind::NearFamily:
		{
			bAny = true;
			const ASLPlaceable* Other = GetHome(N.FamilyIndex);
			if (!Other || Other->bHeld || Other == Ignore)
			{
				return false;
			}
			D = FootDistance(Cell, FIntPoint(1, 1), Other->CellMin, Other->GetFootprint());
			if (D < 1 || D > N.MaxCells)
			{
				return false;
			}
			break;
		}
		default:
			break;
		}
	}
	return bAny;
}

// --- Otobus ve durak cevresi -----------------------------------------------------------------------

void ASLLevelDirector::UpdateBus()
{
	ASLPlatform* P = GetPlatform();
	if (!P || BusStopType.IsNone())
	{
		return;
	}
	// Yol agina bagli ilk durak: otobus o agda, duraktan en uzak yol karosuna gidip gelir.
	TArray<FIntPoint> Roads;
	FIntPoint StopRoad(-1, -1);
	TSet<FIntPoint> StopRoads;
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : P->GetPlaceables())
	{
		const ASLPlaceable* S = Weak.Get();
		if (!S || S->bHeld || S->TypeId != BusStopType)
		{
			continue;
		}
		for (const FIntPoint& E : S->GetEntranceCells())
		{
			const ASLPlaceable* R = P->FindPlaceableAt(E);
			if (R && R->bFlat)
			{
				StopRoads.Add(E);
				if (StopRoad.X < 0)
				{
					StopRoad = E;
				}
			}
		}
	}
	if (StopRoad.X >= 0)
	{
		P->TypesOnRoadNetwork({ StopRoad }, nullptr, &Roads);
	}

	TArray<FVector> Points;
	TArray<int32> Stops;
	if (Roads.Num() > 0)
	{
		// Duraktan BFS: en uzak karo ve ona giden yol.
		const TSet<FIntPoint> RoadSet(Roads);
		TMap<FIntPoint, FIntPoint> Parent;
		TArray<FIntPoint> Queue = { StopRoad };
		Parent.Add(StopRoad, StopRoad);
		const FIntPoint Dirs[4] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1} };
		for (int32 Q = 0; Q < Queue.Num(); ++Q)
		{
			for (const FIntPoint& D : Dirs)
			{
				const FIntPoint N = Queue[Q] + D;
				if (RoadSet.Contains(N) && !Parent.Contains(N))
				{
					Parent.Add(N, Queue[Q]);
					Queue.Add(N);
				}
			}
		}
		TArray<FIntPoint> Path = { Queue.Last() };
		while (Path.Last() != StopRoad)
		{
			Path.Add(Parent[Path.Last()]);
		}
		Algo::Reverse(Path); // durak -> uc
		for (int32 I = 0; I < Path.Num(); ++I)
		{
			Points.Add(FVector(P->CellToLocal(Path[I].X + 0.5f), P->CellToLocal(Path[I].Y + 0.5f), 0.f));
			if (StopRoads.Contains(Path[I]))
			{
				Stops.Add(I);
			}
		}
	}

	if (!Bus && Points.Num() > 0)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Bus = GetWorld()->SpawnActor<ASLBus>(ASLBus::StaticClass(), FTransform::Identity, Params);
		if (Bus)
		{
			Bus->AttachToComponent(P->TiltPivot, FAttachmentTransformRules::KeepRelativeTransform);
			UE_LOG(LogSLLevel, Log, TEXT("SL_BUS otobus basladi, rota %d karo"), Points.Num());
		}
	}
	if (Bus)
	{
		Bus->SetRoute(Points, Stops);
	}
}

void ASLLevelDirector::UpdateNearZone()
{
	ASLPlatform* P = GetPlatform();
	const USLBuildComponent* B = GetBuild();
	if (!P)
	{
		return;
	}
	// Elde ev var ve kime ait oldugu belli: o ailenin butun uzaklik isteklerini tutan bos hucreler sari.
	TArray<FIntPoint> Cells;
	const ASLPlaceable* G = B ? B->GetGhost() : nullptr;
	const int32 Fam = (G && !G->IsHidden() && G->Residents > 0) ? FamilyForPlacement(B->GetMoving()) : INDEX_NONE;
	if (Fam != INDEX_NONE)
	{
		const int32 N = P->GetCellCount();
		for (int32 X = 0; X < N; ++X)
		{
			for (int32 Y = 0; Y < N; ++Y)
			{
				const ASLPlaceable* At = P->FindPlaceableAt(FIntPoint(X, Y));
				if ((!At || At == B->GetMoving()) && CellSatisfiesDistanceNeeds(Fam, FIntPoint(X, Y), B->GetMoving()))
				{
					Cells.Add(FIntPoint(X, Y));
				}
			}
		}
	}
	P->SetZoneCells(Cells, NearZoneColor, NearZoneOpacity);
}

void ASLLevelDirector::ApplyUnlocks()
{
	USLBuildComponent* B = GetBuild();
	if (!B || UnlockedForTask == CurrentTask)
	{
		return;
	}
	if (UnlockedForTask < 0)
	{
		// Bolum basi: hepsi kapali, gorevler acar.
		for (int32 I = 0; I < B->Slots.Num(); ++I)
		{
			B->SetSlotUnlocked(I, false);
		}
	}
	// Bu goreve kadar (dahil) acilan her sey acik kalir.
	for (int32 T = FMath::Max(0, UnlockedForTask + 1); T <= CurrentTask && Tasks.IsValidIndex(T); ++T)
	{
		for (const FName& Id : Tasks[T].Unlocks)
		{
			B->SetSlotUnlocked(B->FindSlotById(Id), true);
		}
	}
	UnlockedForTask = CurrentTask;
}

int32 ASLLevelDirector::CountConnected(const TArray<FName>& Types) const
{
	const ASLPlatform* P = GetPlatform();
	if (!P)
	{
		return 0;
	}
	const int32 N = P->GetCellCount();
	TArray<int32> Road;     // hucre -> 1 yol
	Road.Init(0, N * N);
	TArray<const ASLPlaceable*> Buildings;
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : P->GetPlaceables())
	{
		const ASLPlaceable* Pl = Weak.Get();
		if (!Pl || Pl->bHeld)
		{
			continue;
		}
		if (Pl->TypeId == TEXT("Yol"))
		{
			if (Pl->CellMin.X >= 0 && Pl->CellMin.X < N && Pl->CellMin.Y >= 0 && Pl->CellMin.Y < N)
			{
				Road[Pl->CellMin.Y * N + Pl->CellMin.X] = 1;
			}
		}
		else if (Types.Contains(Pl->TypeId))
		{
			Buildings.Add(Pl);
		}
	}

	// Yol agi bilesenleri (4 komsuluk).
	TArray<int32> Comp;
	Comp.Init(-1, N * N);
	int32 NextComp = 0;
	for (int32 I = 0; I < N * N; ++I)
	{
		if (!Road[I] || Comp[I] >= 0)
		{
			continue;
		}
		TArray<int32> Stack = { I };
		Comp[I] = NextComp;
		while (Stack.Num())
		{
			const int32 Cur = Stack.Pop();
			const int32 X = Cur % N, Y = Cur / N;
			const FIntPoint Nb[4] = { {X + 1, Y}, {X - 1, Y}, {X, Y + 1}, {X, Y - 1} };
			for (const FIntPoint& Q : Nb)
			{
				const int32 Idx = Q.Y * N + Q.X;
				if (Q.X >= 0 && Q.Y >= 0 && Q.X < N && Q.Y < N && Road[Idx] && Comp[Idx] < 0)
				{
					Comp[Idx] = NextComp;
					Stack.Add(Idx);
				}
			}
		}
		++NextComp;
	}

	// Her bilesen icin: hangi hedef turlerine degiyor? En cok ture degen bilesenin sayisi.
	int32 Best = 0;
	for (int32 C = 0; C < NextComp; ++C)
	{
		TSet<FName> Touched;
		for (const ASLPlaceable* Bld : Buildings)
		{
			// Bina yola yalnizca kapisinin onundeki hucreden baglanir; kapisi olmayan binada her yan komsu.
			bool bTouch = false;
			if (Bld->bHasEntrance)
			{
				for (const FIntPoint& E : Bld->GetEntranceCells())
				{
					if (E.X >= 0 && E.Y >= 0 && E.X < N && E.Y < N && Comp[E.Y * N + E.X] == C)
					{
						bTouch = true;
						break;
					}
				}
			}
			else
			{
				const FIntPoint Mn = Bld->CellMin, F = Bld->GetFootprint();
				for (int32 X = Mn.X - 1; X <= Mn.X + F.X && !bTouch; ++X)
				{
					for (int32 Y = Mn.Y - 1; Y <= Mn.Y + F.Y && !bTouch; ++Y)
					{
						const bool bEdgeRing = (X == Mn.X - 1 || X == Mn.X + F.X) != (Y == Mn.Y - 1 || Y == Mn.Y + F.Y);
						if (!bEdgeRing || X < 0 || Y < 0 || X >= N || Y >= N) // kose degil, yan komsu
						{
							continue;
						}
						bTouch = Comp[Y * N + X] == C;
					}
				}
			}
			if (bTouch)
			{
				Touched.Add(Bld->TypeId);
			}
		}
		Best = FMath::Max(Best, Touched.Num());
	}
	return Best;
}

bool ASLLevelDirector::EvaluateTask(const FSLTask& Task, int32& OutProgress, int32& OutGoal) const
{
	const ASLPlatform* P = GetPlatform();
	OutProgress = 0;
	OutGoal = FMath::Max(1, Task.Count);
	if (!P)
	{
		return false;
	}
	switch (Task.Kind)
	{
	case ESLTaskKind::PlaceType:
		for (const TWeakObjectPtr<ASLPlaceable>& Weak : P->GetPlaceables())
		{
			const ASLPlaceable* Pl = Weak.Get();
			OutProgress += (Pl && !Pl->bHeld && Pl->TypeId == Task.TypeId) ? 1 : 0;
		}
		break;
	case ESLTaskKind::FamilyMovedIn:
		for (const TWeakObjectPtr<ASLPlaceable>& Weak : P->GetPlaceables())
		{
			const ASLPlaceable* Pl = Weak.Get();
			OutProgress += (Pl && Pl->TypeId == Task.TypeId && Pl->bResidentsArrived) ? 1 : 0;
		}
		break;
	case ESLTaskKind::ConnectRoads:
		OutGoal = Task.ConnectTypes.Num();
		OutProgress = CountConnected(Task.ConnectTypes);
		break;
	case ESLTaskKind::FamiliesHappy:
		OutGoal = Families.Num();
		for (int32 I = 0; I < Families.Num(); ++I)
		{
			OutProgress += IsFamilyHappy(I) ? 1 : 0;
		}
		for (const FName& T : RequiredTypes)
		{
			if (!IsRequiredPlaced(T))
			{
				return false; // mahalle plani eksik
			}
		}
		break;
	}
	return OutProgress >= OutGoal;
}

void ASLLevelDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (LevelNumber == 0)
	{
		SpawnMenuTown();
	}
	SpawnFamilies();
	ApplyUnlocks();
	UpdateConnectionFx();
	{
		// Balon degisince kisa miriltı (aile konusuyor).
		FString Speech;
		FVector Anchor;
		const bool bHas = GetSpeech(Speech, Anchor);
		if (bHas && Speech != LastSpeech)
		{
			ASLGameMode::PlaySfx(this, TEXT("SFX_Konusma"), 0);
		}
		LastSpeech = bHas ? Speech : FString();
	}
	StartFamilies();
	UpdateHappiness(DeltaSeconds);
	UpdateChatter(DeltaSeconds);
	UpdateBus();
	UpdateNearZone();
	CheckMoveLimit(DeltaSeconds);
	if (bLevelComplete || !Tasks.IsValidIndex(CurrentTask))
	{
		return;
	}
	const ASLPlatform* P = GetPlatform();
	if (P && P->IsCollapsed())
	{
		return;
	}

	// Gorevler sirayla; bir karede birden fazla gorev bitebilir (ornegin once yol dosenmisse).
	while (Tasks.IsValidIndex(CurrentTask) && EvaluateTask(Tasks[CurrentTask], Progress, ProgressGoal))
	{
		const bool bLast = CurrentTask == Tasks.Num() - 1;
		if (bLast && bRequireBalanceToComplete && P && P->BalanceState >= ESLBalanceState::Failing)
		{
			return; // son gorev tamam ama platform devriliyor: once dengele
		}
		UE_LOG(LogSLLevel, Log, TEXT("SL_TASK_DONE %d: %s"), CurrentTask + 1, *Tasks[CurrentTask].Text);
		ASLGameMode::PlaySfx(this, bLast ? TEXT("SFX_Bolum") : TEXT("SFX_Gorev"), bLast ? 5 : 4);
		if (bLast)
		{
			for (ASLFamily* F : FamilyActors)
			{
				if (F)
				{
					F->Hop(); // herkes sevincle ziplar
				}
			}
		}
		if (!Tasks[CurrentTask].DoneSpeech.IsEmpty())
		{
			DoneText = Tasks[CurrentTask].DoneSpeech;
			DoneSpeaker = Tasks[CurrentTask].Speaker;
			DoneUntil = GetWorld()->GetTimeSeconds() + DoneSpeechSeconds;
		}
		++CurrentTask;
		ApplyUnlocks();
		if (bLast)
		{
			CompleteLevel();
			return;
		}
	}
}

void ASLLevelDirector::CompleteLevel()
{
	bLevelComplete = true;
	if (ASLGameMode* GM = GetWorld()->GetAuthGameMode<ASLGameMode>())
	{
		GM->HandleLevelComplete();
	}
	UE_LOG(LogSLLevel, Log, TEXT("SL_LEVEL_COMPLETE %s"), *LevelTitle);
}
