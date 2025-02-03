import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.rmi.Naming;
import java.rmi.RemoteException;
import java.rmi.server.UnicastRemoteObject;

public class GestionComptes implements ICompte {
    private static final String chemin_fichier = "./listeComptes.txt";


    private boolean pseudoDejaUtilise(String pseudo) throws RemoteException {
        try (BufferedReader bufferReader = new BufferedReader(new FileReader(new File(chemin_fichier)))) {
            String ligne;
            while ((ligne = bufferReader.readLine()) != null) {
                String[] elements = ligne.split(",");
                String pseudoExistant = elements[0];
                if (pseudoExistant.equals(pseudo)) {
                    return true; // Pseudo déjà utilisé
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
            throw new RemoteException("Erreur lors de la vérification du pseudo.");
        }
        return false; // Pseudo non utilisé
    }

    @Override
    public boolean creerCompte(String pseudo, String mdp) throws RemoteException {
        if (pseudoDejaUtilise(pseudo)) {
            return false; // Pseudo déjà utilisé
        }

        try (BufferedWriter bufferWriter = new BufferedWriter(new FileWriter(new File(chemin_fichier), true))) {
            bufferWriter.write(pseudo + "," + mdp);
            bufferWriter.newLine();
            return true; // Compte créé avec succès
        } catch (IOException e) {
            e.printStackTrace();
            throw new RemoteException("Erreur lors de la création du compte.");
        }
    }

    @Override
    public boolean supprimerCompter(String pseudo, String mdp) throws RemoteException {
        if (!connexion(pseudo, mdp)) {
            return false; // Le compte n'existe pas
        }

        try (BufferedReader bufferReader = new BufferedReader(new FileReader(new File(chemin_fichier)));
                BufferedWriter bufferWriter = new BufferedWriter(new FileWriter(new File("./tempFile.txt")))) {
            String ligne;
            while ((ligne = bufferReader.readLine()) != null) {
                String[] elements = ligne.split(",");
                String pseudoExistant = elements[0];
                String mdpExistant = elements[1];
                if (!pseudoExistant.equals(pseudo) || !mdpExistant.equals(mdp)) {
                    bufferWriter.write(ligne);
                    bufferWriter.newLine();
                }
            }
            new File(chemin_fichier).delete();
            new File("./tempFile.txt").renameTo(new File(chemin_fichier));
            return true; // Compte supprimé avec succès
        } catch (IOException e) {
            e.printStackTrace();
            throw new RemoteException("Erreur lors de la suppression du compte.");
        }
    }

    @Override
    public boolean connexion(String pseudo, String mdp) throws RemoteException {
        try (BufferedReader bufferReader = new BufferedReader(new FileReader(new File(chemin_fichier)))) {
            String ligne;
            while ((ligne = bufferReader.readLine()) != null) {
                String[] elements = ligne.split(",");
                String pseudoExistant = elements[0];
                String mdpExistant = elements[1];
                if (pseudoExistant.equals(pseudo) && mdpExistant.equals(mdp)) {
                    return true; // Connexion réussie
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
            throw new RemoteException("Erreur lors de la validation de la connexion.");
        }
        return false; // Aucun compte trouvé avec le pseudo et le mot de passe spécifiés
    }

    /**
     * Le constructeur doit préciser que l'exception RemoteException
     * peut être levée
     */
    @SuppressWarnings("deprecation")
    public GestionComptes() throws RemoteException {
	// exportation de l'objet afin de créer la mécanique TCP pour
	// pouvoir appeler les opérations à distance sur cet objet
	UnicastRemoteObject.exportObject(this);
    }

    public static void main(String argv[]) {
	try {
		// on instancie normalement l'objet implémentant les services
		GestionComptes compte = new GestionComptes();

		// on l'enregistre auprès du registry local sous le nom "opRect"
		Naming.rebind("opCpt", compte);

		// si l'enregistrement s'est bien déroulé, cet objet est prêt
		// à recevoir des requêtes d'appels de services
	} catch (Exception e) {
		System.err.println(e);
	}
    }
}
