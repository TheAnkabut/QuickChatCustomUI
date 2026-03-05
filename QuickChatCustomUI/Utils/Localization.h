#pragma once
#include <string>
#include <unordered_map>

namespace Localization
{
    inline bool isSpanish = false;

    inline void Initialize(std::shared_ptr<GameWrapper> wrapper)
    {
        isSpanish = (wrapper->GetUILanguage().ToString() == "ESN");
    }

    inline const char* Get(const char* key)
    {
        // Keys that are the same in both languages
        static const std::unordered_map<std::string, const char*> SHARED = {
            {"presets", "Presets"},
            {"fade", "Fade"},
            {"linear", "Linear"},
            {"smooth", "Smooth"},
            {"color", "Color"},
            {"sync", "Sync"},
        };

        // Spanish translations
        static const std::unordered_map<std::string, const char*> ES = {
            // Header
            {"join_discord", "¡Haz clic para unirte al Discord!"},
            {"new", "Nuevo"},
            {"rename", "Renombrar"},
            {"delete", "Eliminar"},
            {"show_multimedia", "Mostrar Multimedia"},
            {"groups", "Grupos"},
            {"active", "(activo)"},
            {"duration", "Duración"},
            {"duration_tooltip", "Tiempo que se muestra el overlay"},
            {"global", "Global"},
            {"position", "Posición (x, y)"},
            {"scale", "Escala"},
            {"linear_tooltip", "Fade directo 1:1"},
            {"smooth_tooltip", "Fade perceptual con corrección gamma"},
            {"fade_in", "Fade In"},
            {"fade_out", "Fade Out"},
            {"fade_in_tooltip", "Tiempo de aparición del overlay"},
            {"fade_out_tooltip", "Tiempo de desaparición tras seleccionar"},
            {"timeout_tooltip", "Tiempo de desaparición si no se selecciona nada"},

            // Media Panel
            {"multimedia_manager", "Gestor Multimedia"},
            {"no_file", "Sin archivo"},
            {"import", "Importar"},
            {"opacity", "Opacidad"},
            {"roundness", "Redondez"},
            {"speed", "Velocidad"},
            {"group", "Grupo"},
            {"qc_group", "QC Group"},
            {"all", "Todos"},
            {"no_object_selected", "No hay objeto seleccionado"},

            // Text Panel
            {"quickchat_texts", "Quick Chat Textos"},
            {"more_texts", "Más Textos"},
            {"size", "Tamaño"},
            {"shadow", "Sombra"},
            {"click_plus_create", "Haz clic en '+' para crear un texto"},

            // QuickChat Panel
            {"spacing", "Separación"},
            {"selected", "Selec."},
            {"selected_tooltip", "Color cuando se selecciona el Quick Chat"},

            // Welcome
            {"create_first_preset", "Crea tu primer preset para empezar:"},
            {"create", "Crear"},

            // Instruction
            {"quickchat_instruction", "Para usar el plugin, ve a:"},
            {"open_settings", "Configuración > Chat > Ver/Cambiar Chat Rápido"},

            // Common
            {"none", "(ninguno)"},
            {"empty", "(vacío)"},
            {"quickchats", "Quick Chats"},
            {"offset", "Posición (x, y)"},
        };
         
        // English translations
        static const std::unordered_map<std::string, const char*> EN = {
            // Header
            {"join_discord", "Click to join the Discord!"},
            {"new", "New"},
            {"rename", "Rename"},
            {"delete", "Delete"},
            {"show_multimedia", "Show Multimedia"},
            {"groups", "Groups"},
            {"active", "(active)"},
            {"duration", "Duration"},
            {"duration_tooltip", "How long the overlay is displayed"},
            {"global", "Global"},
            {"position", "Position (x, y)"},
            {"scale", "Scale"},
            {"linear_tooltip", "Direct 1:1 fade"},
            {"smooth_tooltip", "Perceptual fade with gamma correction"},
            {"fade_in", "Fade In"},
            {"fade_out", "Fade Out"},
            {"fade_in_tooltip", "Time for overlay to appear"},
            {"fade_out_tooltip", "Time to fade after selection"},
            {"timeout_tooltip", "Time to fade if nothing is selected"},

            // Media Panel
            {"multimedia_manager", "Multimedia Manager"},
            {"no_file", "No file"},
            {"import", "Import"},
            {"opacity", "Opacity"},
            {"roundness", "Roundness"},
            {"speed", "Speed"},
            {"group", "Group"},
            {"qc_group", "QC Group"},
            {"all", "All"},
            {"no_object_selected", "No object selected"},

            // Text Panel
            {"quickchat_texts", "Quick Chat Texts"},
            {"more_texts", "More Texts"},
            {"size", "Size"},
            {"shadow", "Shadow"},
            {"click_plus_create", "Click '+' to create a text"},

            // QuickChat Panel
            {"spacing", "Spacing"},
            {"selected", "Selec."},
            {"selected_tooltip", "Color when Quick Chat is selected"},

            // Welcome
            {"create_first_preset", "Create your first preset to get started:"},
            {"create", "Create"},

            // Instruction
            {"quickchat_instruction", "To use this plugin, go to:"},
            {"open_settings", "Settings > Chat > View/Change Quick Chat"},

            // Common
            {"none", "(none)"},
            {"empty", "(empty)"},
            {"quickchats", "Quick Chats"},
            {"offset", "Position (x, y)"},
        };

        // Check shared first (same in both languages)
        auto shared = SHARED.find(key);
        if (shared != SHARED.end())
        {
            return shared->second;
        }

        const auto& dict = isSpanish ? ES : EN;
        auto it = dict.find(key);
        return (it != dict.end()) ? it->second : key;
    }
}

#define L(key) Localization::Get(key)
