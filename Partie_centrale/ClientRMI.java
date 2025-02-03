
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.rmi.Naming;
import java.rmi.NotBoundException;
import java.rmi.RemoteException;
import java.net.MalformedURLException;

public class ClientRMI {

    public static void main(String[] args) {
        if(args.length != 1){
            System.err.println("Usage: Java ClientRMI <nom_GestionComptes>");
            return;
        }
        try {
            ICompte opCpt = (ICompte) Naming.lookup("rmi://"+ args[0] +":1099/opCpt");

            DatagramPacket packet;

            try {
                DatagramSocket socket = new DatagramSocket(7777);

                while (true) {
                    byte[] buffer = new byte[1024];
                    // Création d'un paquet en utilisant le tableau d'octets
                    packet = new DatagramPacket(buffer, buffer.length);

                    socket.receive(packet);

                    String message = new String(packet.getData(), 0, packet.getLength());

                    // Traitement du message
                    String[] parts = message.split("_");
                    String action = parts[0];

                    try {
                        if (action.equals("CREERCOMPTE")) {
                            String pseudo = parts[1];
                            String mdp = parts[2];
                            // Appeler la méthode creerCompte de la classe GestionComptes
                            boolean compteCree = opCpt.creerCompte(pseudo, mdp);

                            // Préparer la réponse
                            String reponse = compteCree ? "CREERCOMPTE_VALIDE" : "CREERCOMPTE_INVALIDE";

                            // Envoyer la réponse au client
                            byte[] reponseBytes = reponse.getBytes();
                            InetAddress clientAddress = packet.getAddress();
                            int clientPort = packet.getPort();
                            DatagramPacket responsePacket = new DatagramPacket(reponseBytes, reponseBytes.length, clientAddress, clientPort);
                            socket.send(responsePacket);

                        } else if (action.equals("SUPPRIMERCOMPTE")) {
                            String pseudo = parts[1];
                            String mdp = parts[2];
                            // Appel à la méthode supprimerCompte de la classe GestionComptes
                            boolean compteSuppr = opCpt.supprimerCompter(pseudo, mdp);

                            // Préparer la réponse
                            String reponse = compteSuppr ? "SUPPRIMERCOMPTE_VALIDE" : "SUPPRIMERCOMPTE_INVALIDE";
                            
                            // et envoi de la réponse au client
                            byte[] reponseBytes = reponse.getBytes();
                            InetAddress clientAddress = packet.getAddress();
                            int clientPort = packet.getPort();
                            DatagramPacket responsePacket = new DatagramPacket(reponseBytes, reponseBytes.length, clientAddress, clientPort);
                            socket.send(responsePacket);

                        } else if (action.equals("CONNEXION")) {
                            String pseudo = parts[1];
                            String mdp = parts[2];
                            // Appel à la méthode connexion de la classe GestionComptes
                            boolean compteConect = opCpt.connexion(pseudo, mdp);

                            // Préparer la réponse
                            String reponse = compteConect ? "CONNEXION_VALIDE" : "CONNEXION_INVALIDE";
                            // et envoi de la réponse au client
                            byte[] reponseBytes = reponse.getBytes();
                            InetAddress clientAddress = packet.getAddress();
                            int clientPort = packet.getPort();
                            DatagramPacket responsePacket = new DatagramPacket(reponseBytes, reponseBytes.length, clientAddress, clientPort);
                            socket.send(responsePacket);
                        } else{
                            String reponse = "Erreur - <CREERCOMPTE | SUPPRIMERCOMPTE | CONNEXION> <pseudo> <age>";
                            // et envoi de la réponse au client
                            byte[] reponseBytes = reponse.getBytes();
                            InetAddress clientAddress = packet.getAddress();
                            int clientPort = packet.getPort();
                            DatagramPacket responsePacket = new DatagramPacket(reponseBytes, reponseBytes.length, clientAddress, clientPort);
                            socket.send(responsePacket);
                        }
                        //socket.close();
                    } catch (RemoteException e) {
                        e.printStackTrace();
                        // Récupérer le message d'erreur et l'envoyer au client
                        String errorMessage = "Erreur à distance : " + e.getMessage();
                        byte[] errorBytes = errorMessage.getBytes();
                        InetAddress clientAddress = packet.getAddress();
                        int clientPort = packet.getPort();
                        DatagramPacket errorPacket = new DatagramPacket(errorBytes, errorBytes.length, clientAddress, clientPort);
                        socket.send(errorPacket);
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
                // Gérer l'exception globale et arrêter le serveur si nécessaire
            }
        } catch (NotBoundException | MalformedURLException | RemoteException e) {
            e.printStackTrace();
            System.err.println(e);
            // Gérer l'exception si la liaison RMI a échoué
        }
    }
}