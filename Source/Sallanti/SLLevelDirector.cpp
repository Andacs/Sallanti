#include "SLLevelDirector.h"

#include "SLBuildComponent.h"
#include "SLFamily.h"
#include "SLGameMode.h"
#include "SLPlaceable.h"
#include "SLPlatform.h"
#include "SLPlayerController.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogSLLevel, Log, All);

ASLLevelDirector::ASLLevelDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics; // yerlestirme ayni karede gorulsun

	// 1. bolum (egitim / vertical slice): aile gelir, eve yerlesir; hastane, AVM; yollarla baglanir.
	auto Task = [](const TCHAR* Text, const TCHAR* Speech, const TCHAR* Done, const TCHAR* Hint, ESLTaskKind Kind, const TCHAR* Type, std::initializer_list<FName> Unlocks)
	{
		FSLTask T;
		T.Text = Text;
		T.Speech = Speech;
		T.DoneSpeech = Done;
		T.Hint = Hint;
		T.Kind = Kind;
		T.TypeId = Type;
		T.Unlocks = Unlocks;
		return T;
	};
	Tasks.Add(Task(TEXT("Aile için bir ev kur"),
		TEXT("Merhaba başkan! Bu şehre yeni taşındık ama evimiz yok. Bize bir ev yapar mısın?"),
		TEXT("Ne güzel bir ev!"),
		TEXT("Sağdaki çekmeceyi çek, Ev'i seç. Merkeze yakın koy; kapı yönünü R / Q çevirir."),
		ESLTaskKind::PlaceType, TEXT("Ev"), { TEXT("Ev") }));
	Tasks.Add(Task(TEXT("Aile eve yerleşsin"),
		TEXT("Hemen eşyalarımızı taşıyoruz!"),
		TEXT("Yerleştik, çok rahat."),
		TEXT("Aile kapıya yürüyor, biraz bekle."),
		ESLTaskKind::FamilyMovedIn, TEXT("Ev"), {}));
	Tasks.Add(Task(TEXT("Bir hastane kur"),
		TEXT("Çocuklar hastalanırsa nereye gideceğiz? Yakında bir hastane olsa..."),
		TEXT("İçimiz rahatladı."),
		TEXT("Hastaneyi evin karşı tarafına koy: ağırlıklar birbirini dengeler."),
		ESLTaskKind::PlaceType, TEXT("Hastane"), { TEXT("Hastane") }));
	Tasks.Add(Task(TEXT("Bir alışveriş merkezi kur"),
		TEXT("Alışveriş için bir AVM çok iyi olurdu. Ama dikkat, AVM çok ağır!"),
		TEXT("Harika, alışveriş günü!"),
		TEXT("Koymadan önce bara bak: tahmin işareti kırmızıya geçiyorsa başka yer seç."),
		ESLTaskKind::PlaceType, TEXT("AVM"), { TEXT("AVM") }));
	FSLTask Roads = Task(TEXT("Evi, hastaneyi ve AVM'yi yollarla bağla"),
		TEXT("Hastaneye ve AVM'ye yürüyerek gidemiyoruz. Yol yapar mısın?"),
		TEXT(""),
		TEXT("Yol, binaların kapı önündeki sarı karelere değmeli."),
		ESLTaskKind::ConnectRoads, TEXT("Yol"), { TEXT("Yol") });
	Roads.ConnectTypes = { TEXT("Ev"), TEXT("Hastane"), TEXT("AVM") };
	Tasks.Add(Roads);
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

void ASLLevelDirector::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogSLLevel, Log, TEXT("SL_LEVEL %s, %d gorev"), *LevelTitle, Tasks.Num());
}

void ASLLevelDirector::SpawnFamily()
{
	ASLPlatform* P = GetPlatform();
	if (Family || !P)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Family = GetWorld()->SpawnActor<ASLFamily>(ASLFamily::StaticClass(), FTransform::Identity, Params);
	if (Family)
	{
		// Platforma bagli: platform egilince aile de egilir, kutlesi yok.
		Family->AttachToComponent(P->TiltPivot, FAttachmentTransformRules::KeepRelativeTransform);
		Family->SetActorRelativeLocation(FamilyWaitLocal);
		Family->BuildFigures();
	}
}

bool ASLLevelDirector::GetSpeech(FString& OutText, FVector& OutWorldAnchor) const
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	const ASLPlaceable* Home = FamilyHome.Get();
	const bool bShowDone = Now < DoneUntil && !DoneText.IsEmpty();
	if (bLevelComplete)
	{
		OutText = FinalSpeech;
	}
	else if (bShowDone)
	{
		OutText = DoneText;
	}
	else if (Tasks.IsValidIndex(CurrentTask))
	{
		OutText = Tasks[CurrentTask].Speech;
	}
	if (OutText.IsEmpty())
	{
		return false;
	}
	// Aile bekliyorsa onun ustunden, eve yerlestiyse evin ustunden konusur.
	if (Family && Family->IsWaitingVisible())
	{
		OutWorldAnchor = Family->GetSpeechAnchor();
		return true;
	}
	if (Home && !Home->bHeld)
	{
		const FBox B = Home->GetComponentsBoundingBox(false);
		OutWorldAnchor = FVector(B.GetCenter().X, B.GetCenter().Y, B.Max.Z + 40.f);
		return true;
	}
	return false;
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

void ASLLevelDirector::StartFamilies()
{
	const ASLPlatform* P = GetPlatform();
	if (!P)
	{
		return;
	}
	for (const TWeakObjectPtr<ASLPlaceable>& Weak : P->GetPlaceables())
	{
		ASLPlaceable* House = Weak.Get();
		if (!House || House->bHeld || House->Residents <= 0 || House->HasResidentsStarted())
		{
			continue;
		}
		FVector Start;
		if (Family && Family->IsWaitingVisible())
		{
			// Ilk ev: bekleyen aile tam bulundugu yerden yuruyerek gider (figurler el degistirir).
			Start = Family->GetRootComponent()->GetRelativeLocation();
			Family->SetWaitingVisible(false);
			FamilyHome = House;
		}
		else
		{
			// Sonraki evler: kapinin baktigi yondeki kenardan gelen yeni komsular.
			const FVector C = House->GetRootComponent()->GetRelativeLocation();
			const float H = P->HalfExtent - 20.f;
			const FIntPoint D = House->GetFacingDir();
			Start = D.X != 0 ? FVector(D.X * H, C.Y, 0.f) : FVector(C.X, D.Y * H, 0.f);
		}
		House->StartResidentsArrival(Start);
	}
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
	}
	return OutProgress >= OutGoal;
}

void ASLLevelDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	SpawnFamily();
	ApplyUnlocks();
	StartFamilies();
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
		if (!Tasks[CurrentTask].DoneSpeech.IsEmpty())
		{
			DoneText = Tasks[CurrentTask].DoneSpeech;
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
