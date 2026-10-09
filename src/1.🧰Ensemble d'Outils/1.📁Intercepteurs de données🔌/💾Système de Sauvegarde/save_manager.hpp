#ifndef SAVE_MANAGER_HPP
#define SAVE_MANAGER_HPP

#include "save_config.hpp"
#include "save_data.hpp"

#include <godot_cpp/classes/object.hpp>

namespace godot
{
/// 💾 Le service de sauvegarde (classe statique, non instanciable) : point d'entrée unique pour sauvegarder,
/// charger et supprimer des données de jeu. Écrit le résultat de chaque opération dans la console (log_it).
class SaveManager : public Object
{
    GDCLASS(SaveManager, Object);

  protected:
    static void _bind_methods();

  public:
    /// Sauvegarde un SaveData sur disque selon la configuration donnée.
    static void save(const Ref<SaveData> &p_save_data, const Ref<SaveConfig> &p_config);
    /// Charge et retourne les données du fichier correspondant à p_save_data (nul si aucune sauvegarde).
    static Ref<SaveData> load(const Ref<SaveData> &p_save_data, const Ref<SaveConfig> &p_config);
    /// Supprime le fichier de sauvegarde correspondant à p_save_data.
    static void delete_save(const Ref<SaveData> &p_save_data, const Ref<SaveConfig> &p_config);
};
} // namespace godot
#endif