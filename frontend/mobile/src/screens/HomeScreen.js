import React, { useContext } from 'react';
import { View, StyleSheet } from 'react-native';
import { Button, Text, Card, Title, Appbar } from 'react-native-paper';
import { SafeAreaView } from 'react-native-safe-area-context';
import { AuthContext } from '../context/AuthContext';

export default function HomeScreen() {
  const { signOut, user } = useContext(AuthContext);

  const handleLogout = () => {
    signOut();
  };

  return (
    <SafeAreaView style={styles.container}>
      <Appbar.Header>
        <Appbar.Content title="Bill Splitter" />
        <Appbar.Action icon="logout" onPress={handleLogout} />
      </Appbar.Header>

      <View style={styles.content}>
        <Card style={styles.card}>
          <Card.Content style={styles.cardContent}>
            <Title style={styles.welcomeTitle}>Welcome!</Title>
            <Text style={styles.userName}>
              {user?.name} {user?.family_name}
            </Text>
            <Text style={styles.userEmail}>{user?.email}</Text>
          </Card.Content>
        </Card>

        <Card style={styles.card}>
          <Card.Content>
            <Text style={styles.comingSoonTitle}>Coming Soon</Text>
            <Text style={styles.comingSoonText}>
              Bill splitting features will be available soon.
            </Text>
          </Card.Content>
        </Card>

        <Button
          mode="outlined"
          onPress={handleLogout}
          style={styles.logoutButton}
          icon="logout"
        >
          Sign Out
        </Button>
      </View>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  content: {
    flex: 1,
    padding: 20,
  },
  card: {
    marginBottom: 16,
    elevation: 4,
  },
  cardContent: {
    alignItems: 'center',
  },
  welcomeTitle: {
    fontSize: 24,
    marginBottom: 8,
    color: '#2196F3',
  },
  userName: {
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 4,
  },
  userEmail: {
    fontSize: 14,
    color: '#666',
  },
  comingSoonTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    marginBottom: 8,
    textAlign: 'center',
  },
  comingSoonText: {
    fontSize: 14,
    textAlign: 'center',
    color: '#666',
  },
  logoutButton: {
    marginTop: 'auto',
    marginBottom: 20,
  },
});