import java.rmi.Remote;
import java.rmi.RemoteException;

public interface ICompte extends Remote {
    
    /**
     * Création d'un nouveau compte. Le pseudo précisé ne doit pas déjà être
     * utilisé par un autre compte.
     * @param pseudo le pseudo du compte
     * @param mdp le mot de passe du compte
     * @return true si le compte a été créé, false
     * sinon (le pseudo est déjà utilisé)
     */
    boolean creerCompte(String pseudo, String mdp) throws RemoteException;
    
    /**
     * Suppression d'un compte. La précision du mot de passe permet de
     * s'assurer qu'un utilisateur supprime un de ses comptes.
     * @param pseudo le pseudo du compte de l'utilisateur
     * @param mdp le mot de passe du compte de l'utilisateur
     * @return true si la suppression est effective (couple
     * pseudo/mdp valide), false sinon
     */ 
    boolean supprimerCompter(String pseudo, String mdp) throws RemoteException;
    
    /**
     * Validation de la connexion d'un utilisateur au système.
     * @param pseudo le pseudo du compte de l'utilisateur
     * @param mdp le mot de passe du compte de l'utilisateur
     * @return true s'il existe un compte avec le 
     * couple pseudo/mdp précisé, false sinon
     */
    boolean connexion(String pseudo, String mdp) throws RemoteException;
} 