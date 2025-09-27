import React, { useState, useContext } from 'react';
import { View, StyleSheet, ScrollView, Alert } from 'react-native';
import { TextInput, Button, Text, Card, Title } from 'react-native-paper';
import { SafeAreaView } from 'react-native-safe-area-context';
import { AuthContext } from '../context/AuthContext';

export default function RegisterScreen({ navigation }) {
  const [name, setName] = useState('');
  const [familyName, setFamilyName] = useState('');
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [repeatPassword, setRepeatPassword] = useState('');
  const [loading, setLoading] = useState(false);
  const [showPassword, setShowPassword] = useState(false);
  const [showRepeatPassword, setShowRepeatPassword] = useState(false);

  const { signUp } = useContext(AuthContext);

  const handleRegister = async () => {
    if (!name.trim() || !familyName.trim() || !email.trim() || !password.trim() || !repeatPassword.trim()) {
      Alert.alert('Error', 'Please fill in all fields');
      return;
    }

    if (!email.includes('@')) {
      Alert.alert('Error', 'Please enter a valid email address');
      return;
    }

    if (password.length < 6) {
      Alert.alert('Error', 'Password must be at least 6 characters long');
      return;
    }

    if (password !== repeatPassword) {
      Alert.alert('Error', 'Passwords do not match');
      return;
    }

    setLoading(true);
    const result = await signUp(name.trim(), familyName.trim(), email.trim(), password, repeatPassword);
    setLoading(false);

    if (!result.success) {
      Alert.alert('Registration Failed', result.error || 'Something went wrong');
    }
  };

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.scrollContent}>
        <View style={styles.content}>
          <Title style={styles.title}>Bill Splitter</Title>
          <Text style={styles.subtitle}>Create your account</Text>

          <Card style={styles.card}>
            <Card.Content>
              <TextInput
                label="First Name"
                value={name}
                onChangeText={setName}
                mode="outlined"
                autoCapitalize="words"
                style={styles.input}
                disabled={loading}
              />

              <TextInput
                label="Last Name"
                value={familyName}
                onChangeText={setFamilyName}
                mode="outlined"
                autoCapitalize="words"
                style={styles.input}
                disabled={loading}
              />

              <TextInput
                label="Email"
                value={email}
                onChangeText={setEmail}
                mode="outlined"
                keyboardType="email-address"
                autoCapitalize="none"
                autoComplete="email"
                style={styles.input}
                disabled={loading}
              />

              <TextInput
                label="Password"
                value={password}
                onChangeText={setPassword}
                mode="outlined"
                secureTextEntry={!showPassword}
                autoCapitalize="none"
                style={styles.input}
                disabled={loading}
                right={
                  <TextInput.Icon
                    icon={showPassword ? 'eye-off' : 'eye'}
                    onPress={() => setShowPassword(!showPassword)}
                  />
                }
              />

              <TextInput
                label="Repeat Password"
                value={repeatPassword}
                onChangeText={setRepeatPassword}
                mode="outlined"
                secureTextEntry={!showRepeatPassword}
                autoCapitalize="none"
                style={styles.input}
                disabled={loading}
                right={
                  <TextInput.Icon
                    icon={showRepeatPassword ? 'eye-off' : 'eye'}
                    onPress={() => setShowRepeatPassword(!showRepeatPassword)}
                  />
                }
              />

              <Button
                mode="contained"
                onPress={handleRegister}
                style={styles.button}
                loading={loading}
                disabled={loading}
              >
                Sign Up
              </Button>

              <Button
                mode="text"
                onPress={() => navigation.navigate('Login')}
                style={styles.linkButton}
                disabled={loading}
              >
                Already have an account? Sign In
              </Button>
            </Card.Content>
          </Card>
        </View>
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  scrollContent: {
    flexGrow: 1,
    justifyContent: 'center',
  },
  content: {
    padding: 20,
  },
  title: {
    fontSize: 32,
    fontWeight: 'bold',
    textAlign: 'center',
    marginBottom: 8,
    color: '#2196F3',
  },
  subtitle: {
    fontSize: 16,
    textAlign: 'center',
    marginBottom: 32,
    color: '#666',
  },
  card: {
    elevation: 4,
  },
  input: {
    marginBottom: 16,
  },
  button: {
    marginTop: 8,
    paddingVertical: 8,
  },
  linkButton: {
    marginTop: 16,
  },
});