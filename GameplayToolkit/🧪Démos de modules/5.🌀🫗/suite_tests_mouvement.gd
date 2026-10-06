extends "../2.🧭🌍/suite_tests_base.gd"

## Suite de tests automatiques du module Mouvement & Physique (vraie physique, images physiques) :
## déplacement au sol 2D/3D, recul, zones de force, eau et nage, mur, aérodynamique, trémoussement,
## corde, orchestrateur de contextes. Chaque test est placé loin des autres (pas d'interférence).


func _executer() -> void:
	await _tester_sol_2d()
	await _tester_vue_de_dessus_et_rigide()
	await _tester_sol_3d()
	await _tester_recul()
	await _tester_zones_de_force()
	await _tester_eau_et_nage()
	await _tester_mur()
	await _tester_aerodynamique()
	await _tester_tremousse()
	await _tester_corde()
	await _tester_orchestrateur()
	await _tester_3d_divers()


# 🧱 Construction

func _physique(images: int) -> void:
	for i in images:
		await _bac_a_sable.get_tree().physics_frame


func _corps_2d(position: Vector2, rayon := 10.0) -> CharacterBody2D:
	var corps := CharacterBody2D.new()
	corps.position = position
	var forme := CollisionShape2D.new()
	var cercle := CircleShape2D.new()
	cercle.radius = rayon
	forme.shape = cercle
	corps.add_child(forme)
	_ajouter(corps)
	return corps


func _boite_2d(centre: Vector2, taille: Vector2) -> StaticBody2D:
	var boite := StaticBody2D.new()
	boite.position = centre
	var forme := CollisionShape2D.new()
	var rectangle := RectangleShape2D.new()
	rectangle.size = taille
	forme.shape = rectangle
	boite.add_child(forme)
	_ajouter(boite)
	return boite


func _rigide_2d(position: Vector2) -> RigidBody2D:
	var corps := RigidBody2D.new()
	corps.position = position
	var forme := CollisionShape2D.new()
	var cercle := CircleShape2D.new()
	cercle.radius = 8
	forme.shape = cercle
	corps.add_child(forme)
	_ajouter(corps)
	return corps


func _zone_2d(classe: Object, centre: Vector2, taille: Vector2) -> Area2D:
	var zone: Area2D = classe.new()
	zone.position = centre
	var forme := CollisionShape2D.new()
	var rectangle := RectangleShape2D.new()
	rectangle.size = taille
	forme.shape = rectangle
	zone.add_child(forme)
	_ajouter(zone)
	return zone


func _composant(corps: Node, classe: Object) -> Node:
	var composant: Node = classe.new()
	corps.add_child(composant)
	return composant


# 🦶 Sol 2D

func _tester_sol_2d() -> void:
	_boite_2d(Vector2(0, 100), Vector2(2000, 20))
	var corps := _corps_2d(Vector2(0, 0))
	var sol: GroundMovement2D = _composant(corps, GroundMovement2D)
	var atterrissages := _espionner(sol.landed)
	var sauts := _espionner(sol.jumped)
	await _physique(60)
	_verifier("Sol 2D : tombe et atterrit", sol.is_on_floor() and atterrissages.n == 1, str(corps.position))
	sol.move(Vector2(1, 0))
	await _physique(20)
	_verifier("Sol 2D : avance à droite, vitesse plafonnée", corps.position.x > 30 and is_equal_approx(corps.velocity.x, sol.max_speed), str(corps.velocity))
	sol.move(Vector2.ZERO)
	var hauteur := corps.position.y
	sol.jump()
	await _physique(8)
	_verifier("Sol 2D : saut", sauts.n == 1 and corps.position.y < hauteur - 20)
	sol.jump()
	await _physique(2)
	_verifier("Sol 2D : pas de second saut (max_jumps = 1)", sauts.n == 1)
	await _physique(80)
	sol.max_jumps = 2
	sol.jump()
	await _physique(6)
	sol.jump()
	await _physique(2)
	_verifier("Sol 2D : double saut", sauts.n == 3)
	await _physique(100)
	sol.active = false
	sol.move(Vector2(1, 0))
	var x := corps.position.x
	await _physique(10)
	_verifier("Composant inactif : le corps ne bouge plus", absf(corps.position.x - x) < 0.01)


func _tester_vue_de_dessus_et_rigide() -> void:
	var corps := _corps_2d(Vector2(5000, 0))
	var sol: GroundMovement2D = _composant(corps, GroundMovement2D)
	sol.mode = GroundMovement2D.MODE_TOP_DOWN
	sol.move(Vector2(0, 1))
	sol.set_running(true)
	await _physique(30)
	_verifier("Vue de dessus : pas de gravité, course", absf(corps.velocity.y - sol.max_speed * sol.run_multiplier) < 0.01 and sol.get_facing().is_equal_approx(Vector2(0, 1)), str(corps.velocity))
	var rigide := _rigide_2d(Vector2(5200, 0))
	rigide.gravity_scale = 0
	var mouvement_rigide: GroundMovement2D = _composant(rigide, GroundMovement2D)
	mouvement_rigide.mode = GroundMovement2D.MODE_TOP_DOWN
	mouvement_rigide.move(Vector2(1, 0))
	await _physique(30)
	_verifier("RigidBody2D : vitesse pilotée", rigide.linear_velocity.x > 150, str(rigide.linear_velocity))


# 🦶 Sol 3D

func _sol_3d(centre: Vector3, taille := Vector3(40, 1, 40)) -> StaticBody3D:
	var sol := StaticBody3D.new()
	sol.position = centre
	var forme := CollisionShape3D.new()
	var boite := BoxShape3D.new()
	boite.size = taille
	forme.shape = boite
	sol.add_child(forme)
	_ajouter(sol)
	return sol


func _corps_3d(position: Vector3) -> CharacterBody3D:
	var corps := CharacterBody3D.new()
	corps.position = position
	var forme := CollisionShape3D.new()
	forme.shape = CapsuleShape3D.new()
	corps.add_child(forme)
	_ajouter(corps)
	return corps


func _tester_sol_3d() -> void:
	_sol_3d(Vector3(0, -0.5, 1000))
	var corps := _corps_3d(Vector3(0, 2, 1000))
	var sol: GroundMovement3D = _composant(corps, GroundMovement3D)
	await _physique(60)
	_verifier("Sol 3D : atterrit", sol.is_on_floor())
	sol.move(Vector2(1, 0))
	await _physique(40)
	_verifier("Sol 3D : avance sur X, tourné vers la marche", corps.position.x > 1.0 and absf(wrapf(corps.rotation.y, -PI, PI) + PI / 2) < 0.05, "%s %s" % [corps.position, corps.rotation])
	var camera := Camera3D.new()
	_ajouter(camera)
	camera.global_position = corps.global_position + Vector3(5, 3, 0)
	camera.look_at(corps.global_position)
	camera.make_current()
	sol.camera_relative = true
	sol.move(Vector2(0, -1)) # « avant » = vers où regarde la caméra (−X)
	_verifier("Sol 3D : entrée relative à la caméra", sol.get_world_input().x < -0.9, str(sol.get_world_input()))
	camera.queue_free()


# 🌬️ Recul

func _tester_recul() -> void:
	var corps := _corps_2d(Vector2(10000, 0))
	var sol: GroundMovement2D = _composant(corps, GroundMovement2D)
	sol.mode = GroundMovement2D.MODE_TOP_DOWN
	var recul: Knockback2D = _composant(corps, Knockback2D)
	recul.upward_bias = 0.0
	var retours := _espionner(recul.recovered)
	sol.move(Vector2(-1, 0))
	recul.apply_knockback(Vector2(1, 0))
	_verifier("Recul : vitesse imposée, verrouillé", recul.is_knocked() and corps.velocity.x > 400)
	await _physique(5)
	_verifier("Recul : les commandes sont ignorées pendant le recul", corps.velocity.x > 0, str(corps.velocity))
	await _physique(30)
	_verifier("Recul : fin du verrou (recovered)", not recul.is_knocked() and retours.n == 1)

	var seul := _corps_2d(Vector2(10300, 0))
	var recul_seul: Knockback2D = _composant(seul, Knockback2D)
	recul_seul.gravity_affect = false
	recul_seul.upward_bias = 0.0
	recul_seul.apply_knockback_from(Vector2(10200, 0))
	await _physique(10)
	_verifier("Recul sans composant de mouvement : déplace le corps lui-même", seul.position.x > 10320, str(seul.position))

	var rigide := _rigide_2d(Vector2(10600, 0))
	rigide.gravity_scale = 0
	var recul_rigide: Knockback2D = _composant(rigide, Knockback2D)
	recul_rigide.upward_bias = 0.0
	await _physique(1)
	recul_rigide.apply_knockback(Vector2(1, 0))
	await _physique(2)
	_verifier("Recul RigidBody2D : impulsion et rotation", rigide.linear_velocity.x > 300 and absf(rigide.angular_velocity) > 0.0, "%s %s" % [rigide.linear_velocity, rigide.angular_velocity])


# 🧲 Zones de force

func _tester_zones_de_force() -> void:
	var vent: ForceZone2D = _zone_2d(ForceZone2D, Vector2(15000, 0), Vector2(400, 400))
	vent.strength = 500
	var corps := _corps_2d(Vector2(15000, 0))
	var sol: GroundMovement2D = _composant(corps, GroundMovement2D)
	sol.mode = GroundMovement2D.MODE_TOP_DOWN
	sol.deceleration = 0
	await _physique(20)
	_verifier("Vent : un personnage dérive dans la direction", corps.position.x > 15020, str(corps.position))
	_verifier("get_acceleration_on", ForceZone2D.get_acceleration_on(corps).is_equal_approx(Vector2(500, 0)))

	var aimant: ForceZone2D = _zone_2d(ForceZone2D, Vector2(16000, 0), Vector2(600, 600))
	aimant.mode = ForceZone2D.MODE_ATTRACT
	aimant.strength = 800
	var bille := _rigide_2d(Vector2(16150, 0))
	bille.gravity_scale = 0
	await _physique(20)
	_verifier("Aimant : un RigidBody2D est attiré vers le centre", bille.position.x < 16140 and bille.linear_velocity.x < 0, str(bille.position))
	aimant.falloff = ForceZone2D.FALLOFF_LINEAR
	aimant.falloff_radius = 200
	var proche := aimant.compute_acceleration(Vector2(16050, 0), Vector2.ZERO).length()
	var loin := aimant.compute_acceleration(Vector2(16150, 0), Vector2.ZERO).length()
	_verifier("Atténuation linéaire", is_equal_approx(proche, 600) and is_equal_approx(loin, 200), "%s %s" % [proche, loin])
	aimant.mode = ForceZone2D.MODE_VORTEX
	_verifier("Tourbillon : force tangente", aimant.compute_acceleration(Vector2(16100, 0), Vector2.ZERO).normalized().is_equal_approx(Vector2(0, 1)))
	aimant.mode = ForceZone2D.MODE_DRAG
	aimant.strength = 2
	aimant.falloff = ForceZone2D.FALLOFF_NONE
	_verifier("Freinage : opposé à la vitesse", aimant.compute_acceleration(Vector2(16100, 0), Vector2(50, 0)).is_equal_approx(Vector2(-100, 0)))


# 🌊 Eau

func _tester_eau_et_nage() -> void:
	var eau: WaterArea2D = _zone_2d(WaterArea2D, Vector2(20000, 200), Vector2(600, 400))
	_verifier("Eau : surface = haut de la forme", is_equal_approx(eau.get_surface_y(), 0.0), str(eau.get_surface_y()))
	var bouchon := _rigide_2d(Vector2(20000, 250))
	await _physique(180)
	_verifier("Eau : un objet flotte près de la surface", absf(bouchon.position.y) < 30, str(bouchon.position))
	_verifier("find_containing", WaterArea2D.find_containing(bouchon) == eau)

	var nageur := _corps_2d(Vector2(20100, 250))
	var nage: SwimMovement2D = _composant(nageur, SwimMovement2D)
	await _physique(150)
	_verifier("Nage : remonte à la profondeur d'équilibre", absf(eau.get_depth(nageur.global_position) - nage.float_depth) < 12, str(eau.get_depth(nageur.global_position)))
	nage.move(Vector2(0, 1))
	await _physique(40)
	_verifier("Nage : plonge avec la commande", eau.get_depth(nageur.global_position) > 60, str(eau.get_depth(nageur.global_position)))
	nage.move(Vector2.ZERO)
	await _physique(200)
	var sorties := _espionner(nage.jumped_out)
	nage.jump()
	await _physique(3)
	_verifier("Nage : saut hors de l'eau près de la surface", sorties.n == 1 and nageur.velocity.y < -200, str(nageur.velocity))


# 🧗 Mur

func _tester_mur() -> void:
	_boite_2d(Vector2(25040, 0), Vector2(20, 2000))
	var corps := _corps_2d(Vector2(25000, -500))
	var mur: WallMovement2D = _composant(corps, WallMovement2D)
	mur.move(Vector2(1, 0))
	await _physique(60)
	_verifier("Mur : contact et glissade à vitesse limitée", mur.is_touching_wall() and mur.is_sliding() and corps.velocity.y <= mur.slide_speed + 0.01, str(corps.velocity))
	var sauts := _espionner(mur.wall_jumped)
	mur.jump()
	await _physique(3)
	_verifier("Saut mural : loin du mur et vers le haut", sauts.n == 1 and corps.velocity.x < 0 and corps.velocity.y < 0, str(corps.velocity))
	await _physique(40)
	mur.can_climb = true
	mur.move(Vector2(1, 0))
	await _physique(40)
	mur.move(Vector2(0, -1))
	var hauteur := corps.position.y
	await _physique(20)
	_verifier("Escalade : monte le long du mur", mur.is_climbing() and corps.position.y < hauteur - 20, "%s → %s" % [hauteur, corps.position.y])


# 🪂 Aérodynamique

func _tester_aerodynamique() -> void:
	var corps := _corps_2d(Vector2(30000, 0))
	var aero: AerodynamicMovement2D = _composant(corps, AerodynamicMovement2D)
	aero.max_flaps = 1
	aero.set_active(false)
	aero.set_active(true) # recharge les battements
	await _physique(60)
	_verifier("Plané : chute plafonnée", corps.velocity.y <= aero.glide_fall_speed + 0.01 and corps.velocity.y > 0, str(corps.velocity))
	var battements := _espionner(aero.flapped)
	aero.jump()
	await _physique(2)
	aero.jump()
	await _physique(2)
	_verifier("Plané : un battement d'aile (max_flaps = 1)", battements.n == 1 and aero.get_flaps_left() == 0)
	aero.mode = AerodynamicMovement2D.MODE_FLY
	aero.move(Vector2(0, -1))
	await _physique(30)
	_verifier("Vol libre : monte avec la commande", corps.velocity.y < -200, str(corps.velocity))

	var feuille := _rigide_2d(Vector2(31000, 0))
	var traine: AerodynamicMovement2D = _composant(feuille, AerodynamicMovement2D)
	traine.drag_coefficient = 0.02
	var caillou := _rigide_2d(Vector2(31200, 0))
	await _physique(90)
	_verifier("Traînée : chute plus lente qu'un objet sans traînée", feuille.linear_velocity.y < caillou.linear_velocity.y * 0.5, "%s / %s" % [feuille.linear_velocity.y, caillou.linear_velocity.y])


# 🫨 Trémoussement

func _tester_tremousse() -> void:
	var support := Node2D.new()
	support.position = Vector2(35000, 0)
	_ajouter(support)
	var gelee := Jiggle2D.new()
	gelee.position = Vector2(0, -20)
	support.add_child(gelee)
	await _frames(2)
	_verifier("Trémousse : au repos sur sa position", gelee.get_offset().length() < 0.01)
	support.position.x += 60
	await _frames(2)
	_verifier("Trémousse : traîne quand le parent bouge", gelee.get_offset().x < -1, str(gelee.get_offset()))
	_verifier("Trémousse : décalage plafonné", gelee.get_offset().length() <= gelee.max_offset + 0.01)
	await _secondes(1.5)
	_verifier("Trémousse : revient au repos", gelee.get_offset().length() < 0.5, str(gelee.get_offset()))
	gelee.impulse(Vector2(0, 400))
	await _frames(3)
	_verifier("impulse : secoue", gelee.get_offset().length() > 1 and not gelee.scale.is_equal_approx(Vector2.ONE))


# 🪢 Corde

func _tester_corde() -> void:
	var corde := Rope2D.new()
	corde.position = Vector2(40000, 0)
	corde.rope_length = 200
	_ajouter(corde)
	await _physique(60)
	var points := corde.get_points()
	_verifier("Corde : pend sous l'ancrage, longueur respectée", points.size() == 16 and absf(points[15].y - 200) < 10 and absf(points[15].x - 40000) < 1, str(points[15]))

	var fin := Node2D.new()
	fin.position = Vector2(40150, 0)
	_ajouter(fin)
	var pont := Rope2D.new()
	pont.position = Vector2(40000, 300)
	pont.end_anchor_path = pont.get_path_to(fin) if false else NodePath("")
	_ajouter(pont)
	pont.end_anchor_path = pont.get_path_to(fin)
	fin.position = Vector2(40150, 300)
	pont.reset_points()
	await _physique(30)
	_verifier("Corde : extrémité ancrée", pont.get_points()[pont.point_count - 1].distance_to(fin.global_position) < 0.01)

	var corps := _corps_2d(Vector2(40020, 100))
	var grimpe: RopeMovement2D = _composant(corps, RopeMovement2D)
	var prises := _espionner(grimpe.grabbed)
	_verifier("try_grab : s'accroche à la corde proche", grimpe.try_grab() and prises.n == 1 and Rope2D.find_rope_of(corps) == corde)
	await _physique(5)
	_verifier("Corps accroché : suit la corde", corps.global_position.distance_to(corde.get_position_at(corde.get_attached_position(corps))) < 0.01)
	grimpe.move(Vector2(0, -1))
	var position_corde := corde.get_attached_position(corps)
	await _physique(20)
	_verifier("Grimper : remonte la corde", corde.get_attached_position(corps) < position_corde)
	var x_avant := corps.global_position.x
	grimpe.move(Vector2(1, 0))
	await _physique(20)
	_verifier("Balancer : le corps accroché part sur le côté", corps.global_position.x - x_avant > 5, "%s → %s (position %s)" % [x_avant, corps.global_position.x, corde.get_attached_position(corps)])
	grimpe.jump()
	await _physique(2)
	_verifier("Saut : lâche la corde avec un élan vers le haut", not grimpe.is_attached() and corps.velocity.y < 0)
	_verifier("Pas de reprise immédiate (regrab_delay)", not grimpe.try_grab())


# 🎬 Orchestrateur

func _tester_orchestrateur() -> void:
	_boite_2d(Vector2(50000, 100), Vector2(3000, 20))
	var eau: WaterArea2D = _zone_2d(WaterArea2D, Vector2(50800, 300), Vector2(400, 400))
	eau.position = Vector2(50800, -300)
	var corps := _corps_2d(Vector2(50000, 0))
	var sol: GroundMovement2D = _composant(corps, GroundMovement2D)
	var nage: SwimMovement2D = _composant(corps, SwimMovement2D)
	var mur: WallMovement2D = _composant(corps, WallMovement2D)
	var directeur: MovementDirector = _composant(corps, MovementDirector)
	var changements := {"n": 0, "dernier": &""}
	directeur.context_changed.connect(func(_avant, apres): changements.n += 1; changements.dernier = apres)
	await _physique(30)
	var noms: Array = directeur.contexts.map(func(c): return c.context_name)
	_verifier("Contextes par défaut selon les composants", noms.has(&"sol") and noms.has(&"eau") and noms.has(&"mur"), str(noms))
	_verifier("Contexte au sol : seul le sol est actif", directeur.get_current_context() == &"sol" and sol.active and not nage.active and not mur.active)
	directeur.move(Vector2(1, 0))
	_verifier("Intention transmise à tous les composants", sol.get_move_input() == Vector2(1, 0) and nage.get_move_input() == Vector2(1, 0))
	corps.global_position = Vector2(50800, -300)
	await _physique(3)
	_verifier("Dans l'eau : contexte « eau »", directeur.get_current_context() == &"eau" and nage.active and not sol.active, str(directeur.get_environment()))
	directeur.set_context(&"sol")
	_verifier("set_context : contexte forcé", directeur.get_current_context() == &"sol" and directeur.get_forced_context() == &"sol")
	directeur.clear_forced_context()
	await _physique(2)
	_verifier("clear_forced_context : retour à l'automatique", directeur.get_current_context() == &"eau")
	_verifier("context_changed émis", changements.n >= 3, str(changements))

	var personnalise := MovementContext.new()
	personnalise.context_name = &"test"
	personnalise.condition = MovementContext.CONDITION_CUSTOM
	personnalise.expression = "body.position.x > 50700 and director.is_in_water()"
	_verifier("Condition personnalisée (expression)", directeur.is_condition_met(personnalise))


# 🧊 3D divers

func _tester_3d_divers() -> void:
	var eau := WaterArea3D.new()
	var forme := CollisionShape3D.new()
	var boite := BoxShape3D.new()
	boite.size = Vector3(10, 4, 10)
	forme.shape = boite
	eau.add_child(forme)
	eau.position = Vector3(0, -2, 2000)
	_ajouter(eau)
	_verifier("Eau 3D : surface = haut de la boîte", is_equal_approx(eau.get_surface_y(), 0.0))
	var nageur := _corps_3d(Vector3(0, -3, 2000))
	var nage: SwimMovement3D = _composant(nageur, SwimMovement3D)
	await _physique(150)
	_verifier("Nage 3D : remonte vers la surface", absf(eau.get_depth(nageur.global_position) - nage.float_depth) < 0.4, str(eau.get_depth(nageur.global_position)))

	var corde := Rope3D.new()
	corde.position = Vector3(0, 10, 3000)
	corde.rope_length = 3
	_ajouter(corde)
	await _physique(60)
	var fin: Vector3 = corde.get_points()[corde.point_count - 1]
	_verifier("Corde 3D : pend sous l'ancrage", absf(fin.y - 7) < 0.2 and absf(fin.x) < 0.01, str(fin))

	var zone := ForceZone3D.new()
	zone.mode = ForceZone3D.MODE_VORTEX
	zone.strength = 5
	_ajouter(zone)
	_verifier("Tourbillon 3D : autour de Y", zone.compute_acceleration(Vector3(1, 0, 0), Vector3.ZERO).normalized().is_equal_approx(Vector3(0, 0, -1)), str(zone.compute_acceleration(Vector3(1, 0, 0), Vector3.ZERO)))

	var support := Node3D.new()
	support.position = Vector3(0, 0, 4000)
	_ajouter(support)
	var antenne := Jiggle3D.new()
	support.add_child(antenne)
	await _frames(2)
	support.position.x += 1
	await _frames(2)
	_verifier("Trémousse 3D : traîne et s'incline", antenne.get_offset().x < -0.01 and not antenne.transform.basis.is_equal_approx(Basis()), str(antenne.get_offset()))
